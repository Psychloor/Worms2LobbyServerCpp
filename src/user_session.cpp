#include "user_session.hpp"

#include <spdlog/spdlog.h>

#include "database.hpp"
#include "framed_packet_reader.hpp"
#include "game.hpp"
#include "packet_code.hpp"
#include "packet_handler.hpp"
#include "room.hpp"
#include "server.hpp"
#include "string_utils.hpp"
#include "user.hpp"
#include "worms_packet.hpp"
#include <asio/steady_timer.hpp>

namespace
{
    using namespace worms_server;

    awaitable<void> LeaveRoom(const std::shared_ptr<Room>& room, uint32_t leftId) {
        const auto database    = Database::getInstance();
        const auto users       = database->getUsers();
        const auto games       = database->getGames();
        const uint32_t roomId = room == nullptr ? 0 : room->getId();


        spdlog::debug("User Session: Leaving room {}", roomId);
        const auto hasOtherUsers = std::ranges::any_of(users, [leftId, roomId](const auto& user) {
            return user->getId() != leftId && user->getRoomId() == roomId;
        });

        const auto hasOtherGames = std::ranges::any_of(games, [leftId, roomId](const auto& game) {
            return game->getId() != leftId && game->getRoomId() == roomId;
        });

        const bool roomClosed = room && !hasOtherUsers && !hasOtherGames;

        if (roomClosed) {
            database->removeRoom(roomId);
        }

        const auto roomLeavePacketBytes =
            WormsPacket::freeze(PacketCode::Leave, {.value2 = roomId, .value10 = leftId});
        const auto roomClosePacketBytes = WormsPacket::freeze(PacketCode::Close, {.value10 = roomId});

        for (const auto& user : users) {
            if (user->getId() == leftId) {
                continue;
            }

            if (room != nullptr) {
                user->sendPacket(roomLeavePacketBytes);
            }

            if (roomClosed) {
                user->sendPacket(roomClosePacketBytes);
            }
        }

        co_return;
    }

    awaitable<void> DisconnectUser(const std::shared_ptr<User>& client_user) {
        if (client_user == nullptr) {
            co_return;
        }

        spdlog::debug("User Session: Disconnecting user {}", client_user->getName());

        uint32_t leftId = client_user->getId();

        if (leftId == 0) {
            co_return;
        }

        uint32_t roomId    = client_user->getRoomId();
        const auto database = Database::getInstance();
        database->removeUser(leftId);

        // Close abandoned game
        if (const auto game = database->getGameByName(client_user->getName())) {
            roomId = game->getRoomId();
            leftId = game->getId();

            database->removeGame(leftId);

            const auto roomLeavePacketBytes =
                WormsPacket::freeze(PacketCode::Leave, {.value2 = game->getId(), .value10 = client_user->getId()});
            const auto roomClosePacketBytes = WormsPacket::freeze(PacketCode::Close, {.value10 = game->getId()});
            for (const auto& user : database->getUsers()) {
                if (user->getId() == client_user->getId()) {
                    continue;
                }

                user->sendPacket(roomLeavePacketBytes);
                user->sendPacket(roomClosePacketBytes);
            }
        }

        // Close abandoned room
        co_await LeaveRoom(database->getRoom(roomId), leftId);

        // Notify other users we've disconnected
        const auto packetBytes =
            WormsPacket::freeze(PacketCode::DisconnectUser, {.value10 = client_user->getId()});
        for (const auto& user : database->getUsers()) {
            user->sendPacket(packetBytes);
        }

        co_return;
    }
}

namespace worms_server {


    UserSession::UserSession(ip::tcp::socket socket)
        : database_(Database::getInstance()), socket_(std::move(socket)), timer_(socket_.get_executor()),
          strand_(socket_.get_executor()) {
        timer_.expires_at(std::chrono::steady_clock::time_point::max());
        Server::connectionCount.fetch_add(1, std::memory_order_relaxed);
    }

    UserSession::~UserSession() {
        isShuttingDown_ = true;
        socket_.close();

        Server::connectionCount.fetch_sub(1, std::memory_order_relaxed);

        // Clear any pending packets
        moodycamel::ConsumerToken consumer_token(packets_);
        net::shared_bytes_ptr bytes;
        while (packets_.try_dequeue(consumer_token, bytes)) {
            // Drain the queue
        }

        spdlog::debug("User session for {} destroyed", user_ ? user_->getName() : "unknown");
    }

    awaitable<void> UserSession::run() {
        // Keep ref alive for the full coroutine lifetime
        [[maybe_unused]] auto self = shared_from_this();

        co_spawn(strand_, [self = shared_from_this()]() -> awaitable<void> { co_await self->writer(); }, detached);

        user_ = co_await handleLogin();
        if (user_ == nullptr) {
            spdlog::error("Failed to login");
            co_return;
        }

        spdlog::info("User {} logged in", user_->getName());
        co_await handleSession();


        co_await DisconnectUser(user_);
        co_return;
    }

    void UserSession::sendPacket(const net::shared_bytes_ptr& packet) {
        const moodycamel::ProducerToken producerToken(packets_);
        packets_.enqueue(producerToken, packet);
        timer_.cancel_one(); // wake up the writer if sleeping
    }

    ip::address_v4 UserSession::addressV4() const {
        return socket_.remote_endpoint().address().to_v4();
    }

    awaitable<void> UserSession::writer() {
        try {
            moodycamel::ConsumerToken consumerToken(packets_);
            static constexpr auto FLUSH_DELAY = std::chrono::milliseconds(100);

            std::vector<net::shared_bytes_ptr> packetBatch;
            std::vector<const_buffer> buffers;

            net::shared_bytes_ptr packet;

            while (!isShuttingDown_) {
                // Collect available packets
                while (packets_.try_dequeue(consumerToken, packet) && packetBatch.size() < 16) {
                    packetBatch.push_back(std::move(packet));
                }


                // Flush everything currently queued
                if (!packetBatch.empty()) {
                    // Prepare buffers for vectored writing
                    buffers.clear();
                    for (const auto& pkt : packetBatch) {
                        buffers.emplace_back(pkt->data(), pkt->size());
                    }

                    error_code ec;
                    co_await async_write(socket_, buffers, redirect_error(use_awaitable, ec));


                    if (ec) {
                        co_return; // socket closed/reset
                    }
                    packetBatch.clear();
                }

                error_code ec;
                if (packets_.size_approx() == 0) {
                    timer_.expires_after(FLUSH_DELAY);
                    co_await timer_.async_wait(redirect_error(use_awaitable, ec));
                }

                if (ec == error::operation_aborted) {
                    continue; // packet arrived
                }
                if (ec) {
                    co_return; // io_context stopped
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("writer coroutine error: {}", e.what());
        }
        co_return;
    }

    awaitable<std::shared_ptr<User>> UserSession::handleLogin() {
        std::vector<std::byte> incoming(1024);

        steady_timer timer(socket_.get_executor());
        timer.expires_after(std::chrono::seconds(3));


        try {
            uint32_t userId = 0;
            // Start the timer
            timer.async_wait([&](const error_code& wait_ec) {
                if (!wait_ec) {
                    // Timer expired, close the socket
                    socket_.close();
                }
            });

            // Wait for the client to send a login packet
            error_code ec;

            co_await socket_.async_receive(buffer(incoming), redirect_error(use_awaitable, ec));

            // Cancel the timer since we got data
            timer.cancel();

            if (ec) {
                spdlog::error("Error reading login packet: {}", ec.message());
                co_return nullptr;
            }

            auto packetReader = net::packet_reader(incoming);
            auto [status, data, error]  = WormsPacket::readFrom(packetReader);
            if (status == net::packet_parse_status::error) {
                spdlog::error("Error reading login packet: {}", error.value_or(""));
                co_return nullptr;
            }

            if (status == net::packet_parse_status::partial) {
                spdlog::error("Login packet is empty");
                co_return nullptr;
            }
            if (!data) {
                spdlog::error("Login packet has no data");
                co_return nullptr;
            }

            const auto& login_info = *data;
            if (login_info->code() != PacketCode::Login) {
                spdlog::error("Invalid packet code in login packet");
                co_return nullptr;
            }

            if (const auto& fields = login_info->fields();
                !fields.value1 || !fields.value4 || !fields.name || !fields.info) {
                spdlog::error("Not enough data in login packet");
                co_return nullptr;
            }


            const std::string_view username = *login_info->fields().name;

            // check if a username is valid and not already taken
            const auto currentUsers = database_->getUsers();
            const auto foundUser    = std::ranges::find_if(currentUsers,
                   [&username](const auto& user) { return EqualsCaseInsensitive(user->getName(), username); });

            if (foundUser != currentUsers.end()) {
                const auto bytes = WormsPacket::freeze(PacketCode::LoginReply, {.value1 = 0, .error = 1});
                co_await socket_.async_write_some(buffer(bytes->data(), bytes->size()), use_awaitable);
                co_return nullptr;
            }

            // create a new user and add it to the database
            userId = Database::getNextId();
            const auto clientUser =
                std::make_shared<User>(shared_from_this(), userId, username, login_info->fields().info->playerNation);

            // Notify other users we've logged in
            const auto packetBytes = WormsPacket::freeze(PacketCode::Login,
                {.value1 = userId, .value4 = 0, .name = username.data(), .info = clientUser->getSessionInfo()});
            for (const auto& user : database_->getUsers()) {
                user->sendPacket(packetBytes);
            }

            database_->addUser(clientUser);

            // Send the login reply packet
            sendPacket(WormsPacket::freeze(PacketCode::LoginReply, {.value1 = userId, .error = 0}));

            co_return std::move(clientUser);
        } catch (std::exception& e) {
            spdlog::error("Error in User Session login: {}", e.what());
            co_return nullptr;
        }
    }

    awaitable<void> UserSession::handleSession() {
        try {
            net::framed_packet_reader<WormsPacketPtr, std::string> reader(WormsPacket::readFrom);

            static constexpr auto TIMEOUT_DELAY = std::chrono::minutes(10);
            const string_view username   = user_->getName();

            steady_timer timer(socket_.get_executor());
            std::vector<std::byte> incoming(2048);
            while (socket_.is_open()) {
                try {
                    timer.expires_after(TIMEOUT_DELAY);
                    timer.async_wait([&](const error_code& wait_ec) {
                        if (!wait_ec && socket_.is_open()) {
                            // Timer expired, close the socket
                            socket_.close();
                        }
                    });

                    error_code ec;
                    const size_t read =
                        co_await socket_.async_receive(buffer(incoming), redirect_error(use_awaitable, ec));

                    // Cancel the timer since we got data
                    timer.cancel();


                    if (read == 0 || ec == error::eof) {
                        spdlog::info("User {} disconnected", username);
                        break;
                    }
                    if (ec) {
                        spdlog::error("Error receiving data: {}", ec.message());
                        break;
                    }

                    reader.append(incoming.data(), read);
                    while (true) {
                        const auto [status, data, error] = reader.try_read_packet();
                        if (status == net::packet_parse_status::partial) {
                            // Needs more data
                            break;
                        }

                        if (status == net::packet_parse_status::error) {
                            // Invalid data
                            spdlog::error("Parse error: {}", error.value_or(""));
                            co_return;
                        }

                        spdlog::debug(
                            "Received packet code {} from {}", static_cast<uint32_t>(data.value()->code()), username);

                        if (!co_await PacketHandler::handlePacket(user_, database_, std::move(*data))) {
                            spdlog::warn("Packet handler failed or returned false");
                            co_return;
                        }
                    }
                } catch (const std::exception& e) {
                    spdlog::error("Error handling packet: {}", e.what());
                    // Continue instead of breaking to be more resilient
                    continue;
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("Fatal error in User Session: {}", e.what());
        }

        co_return;
    }
} // namespace worms_server
