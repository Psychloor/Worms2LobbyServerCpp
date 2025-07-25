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

namespace worms_server {
    static awaitable<void> leave_room(const std::shared_ptr<room>& room, uint32_t left_id) {
        const auto database    = database::get_instance();
        const auto users       = database->get_users();
        const auto games       = database->get_games();
        const uint32_t room_id = room == nullptr ? 0 : room->get_id();


        spdlog::debug("User Session: Leaving room {}", room_id);
        const auto has_other_users = std::ranges::any_of(users, [left_id, room_id](const auto& user) {
            return user->get_id() != left_id && user->get_room_id() == room_id;
        });

        const auto has_other_games = std::ranges::any_of(games, [left_id, room_id](const auto& game) {
            return game->get_id() != left_id && game->get_room_id() == room_id;
        });

        const bool room_closed = room && !has_other_users && !has_other_games;

        if (room_closed) {
            database->remove_room(room_id);
        }

        const auto room_leave_packet_bytes =
            worms_packet::freeze(packet_code::leave, {.value2 = room_id, .value10 = left_id});
        const auto room_close_packet_bytes = worms_packet::freeze(packet_code::close, {.value10 = room_id});

        for (const auto& user : users) {
            if (user->get_id() == left_id) {
                continue;
            }

            if (room != nullptr) {
                user->send_packet(room_leave_packet_bytes);
            }

            if (room_closed) {
                user->send_packet(room_close_packet_bytes);
            }
        }

        co_return;
    }

    static awaitable<void> disconnect_user(const std::shared_ptr<user>& client_user) {
        if (client_user == nullptr) {
            co_return;
        }

        spdlog::debug("User Session: Disconnecting user {}", client_user->get_name());

        uint32_t left_id = client_user->get_id();

        if (left_id == 0) {
            co_return;
        }

        uint32_t room_id    = client_user->get_room_id();
        const auto database = database::get_instance();
        database->remove_user(left_id);

        // Close abandoned game
        if (const auto game = database->get_game_by_name(client_user->get_name())) {
            room_id = game->get_room_id();
            left_id = game->get_id();

            database->remove_game(left_id);

            const auto room_leave_packet_bytes =
                worms_packet::freeze(packet_code::leave, {.value2 = game->get_id(), .value10 = client_user->get_id()});
            const auto room_close_packet_bytes = worms_packet::freeze(packet_code::close, {.value10 = game->get_id()});
            for (const auto& user : database->get_users()) {
                if (user->get_id() == client_user->get_id()) {
                    continue;
                }

                user->send_packet(room_leave_packet_bytes);
                user->send_packet(room_close_packet_bytes);
            }
        }

        // Close abandoned room
        co_await leave_room(database->get_room(room_id), left_id);

        // Notify other users we've disconnected
        const auto packet_bytes =
            worms_packet::freeze(packet_code::disconnect_user, {.value10 = client_user->get_id()});
        for (const auto& user : database->get_users()) {
            user->send_packet(packet_bytes);
        }

        co_return;
    }

    user_session::user_session(ip::tcp::socket socket)
        : database_(database::get_instance()), socket_(std::move(socket)), timer_(socket_.get_executor()),
          strand_(socket_.get_executor()) {
        timer_.expires_at(std::chrono::steady_clock::time_point::max());
        server::connection_count.fetch_add(1, std::memory_order_relaxed);
    }

    user_session::~user_session() {
        is_shutting_down_ = true;
        socket_.close();

        server::connection_count.fetch_sub(1, std::memory_order_relaxed);

        // Clear any pending packets
        moodycamel::ConsumerToken consumer_token(packets_);
        net::shared_bytes_ptr bytes;
        while (packets_.try_dequeue(consumer_token, bytes)) {
            // Drain the queue
        }

        spdlog::debug("User session for {} destroyed", user_ ? user_->get_name() : "unknown");
    }

    awaitable<void> user_session::run() {
        // Keep ref alive for the full coroutine lifetime
        auto self = shared_from_this();

        co_spawn(strand_, [self = shared_from_this()]() -> awaitable<void> { co_await self->writer(); }, detached);

        user_ = co_await handle_login();
        if (user_ == nullptr) {
            spdlog::error("Failed to login");
            co_return;
        }

        spdlog::info("User {} logged in", user_->get_name());
        co_await handle_session();


        co_await disconnect_user(user_);
        co_return;
    }

    void user_session::send_packet(const net::shared_bytes_ptr& packet) {
        const moodycamel::ProducerToken producer_token(packets_);
        packets_.enqueue(producer_token, packet);
        timer_.cancel_one(); // wake up the writer if sleeping
    }

    ip::address_v4 user_session::address_v4() const {
        return socket_.remote_endpoint().address().to_v4();
    }

    awaitable<void> user_session::writer() {
        try {
            moodycamel::ConsumerToken consumer_token(packets_);
            constexpr auto flush_delay = std::chrono::milliseconds(100);

            std::vector<net::shared_bytes_ptr> packet_batch;
            std::vector<const_buffer> buffers;

            net::shared_bytes_ptr packet;

            while (!is_shutting_down_) {
                // Collect available packets
                while (packets_.try_dequeue(consumer_token, packet) && packet_batch.size() < 16) {
                    packet_batch.push_back(std::move(packet));
                }


                // Flush everything currently queued
                if (!packet_batch.empty()) {
                    // Prepare buffers for vectored writing
                    buffers.clear();
                    for (const auto& pkt : packet_batch) {
                        buffers.emplace_back(pkt->data(), pkt->size());
                    }

                    error_code ec;
                    co_await async_write(socket_, buffers, redirect_error(use_awaitable, ec));


                    if (ec) {
                        co_return; // socket closed/reset
                    }
                    packet_batch.clear();
                }

                error_code ec;
                if (packets_.size_approx() == 0) {
                    timer_.expires_after(flush_delay);
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

    awaitable<std::shared_ptr<user>> user_session::handle_login() {
        std::vector<std::byte> incoming(1024);

        steady_timer timer(socket_.get_executor());
        timer.expires_after(std::chrono::seconds(3));


        try {
            uint32_t user_id = 0;
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

            auto packet_reader = net::packet_reader(incoming);
            auto login_packet  = worms_packet::read_from(packet_reader);
            if (login_packet.status == net::packet_parse_status::error) {
                spdlog::error("Error reading login packet: {}", login_packet.error.value_or(""));
                co_return nullptr;
            }

            if (login_packet.status == net::packet_parse_status::partial) {
                spdlog::error("Login packet is empty");
                co_return nullptr;
            }
            if (!login_packet.data) {
                spdlog::error("Login packet has no data");
                co_return nullptr;
            }

            const auto& login_info = *login_packet.data;
            if (login_info->code() != packet_code::login) {
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
            const auto current_users = database_->get_users();
            const auto found_user    = std::ranges::find_if(current_users,
                   [&username](const auto& user) { return equals_case_insensitive(user->get_name(), username); });

            if (found_user != current_users.end()) {
                const auto bytes = worms_packet::freeze(packet_code::login_reply, {.value1 = 0, .error = 1});
                co_await socket_.async_write_some(buffer(bytes->data(), bytes->size()), use_awaitable);
                co_return nullptr;
            }

            // create a new user and add it to the database
            user_id = database::get_next_id();
            const auto client_user =
                std::make_shared<user>(shared_from_this(), user_id, username, login_info->fields().info->player_nation);

            // Notify other users we've logged in
            const auto packet_bytes = worms_packet::freeze(packet_code::login,
                {.value1 = user_id, .value4 = 0, .name = username.data(), .info = client_user->get_session_info()});
            for (const auto& user : database_->get_users()) {
                user->send_packet(packet_bytes);
            }

            database_->add_user(client_user);

            // Send the login reply packet
            send_packet(worms_packet::freeze(packet_code::login_reply, {.value1 = user_id, .error = 0}));

            co_return std::move(client_user);
        } catch (std::exception& e) {
            spdlog::error("Error in User Session login: {}", e.what());
            co_return nullptr;
        }
    }

    awaitable<void> user_session::handle_session() {
        try {
            net::framed_packet_reader<worms_packet_ptr, std::string> reader(worms_packet::read_from);

            constexpr auto timeout_delay = std::chrono::minutes(10);
            const string_view username   = user_->get_name();

            steady_timer timer(socket_.get_executor());
            std::vector<std::byte> incoming(2048);
            while (socket_.is_open()) {
                try {
                    timer.expires_after(timeout_delay);
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

                        if (!co_await packet_handler::handle_packet(user_, database_, std::move(*data))) {
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
