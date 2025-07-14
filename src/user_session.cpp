#include "user_session.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include "database.hpp"
#include "packet_code.hpp"
#include "user.hpp"
#include "room.hpp"
#include "game.hpp"
#include "worms_packet.hpp"

#include <spdlog/spdlog.h>

#include "framed_packet_reader.hpp"
#include "packet_handler.hpp"
#include "server.hpp"

namespace worms_server
{
	static awaitable<void> leave_room(const std::shared_ptr<room>& room, uint32_t left_id)
	{
		const auto database = database::get_instance();
		const auto users = database->get_users();
		const uint32_t room_id = room == nullptr ? 0 : room->get_id();

		spdlog::debug("User Session: Leaving room {}", room_id);

		const bool room_closed = room != nullptr
			&& !std::ranges::any_of(users, [left_id, room_id](const auto& user)
			{
				return user->get_id() != left_id && user->get_room_id() == room_id;
			})
			&& !std::ranges::any_of(database->get_games(), [left_id, room_id](const auto& game)
			{
				return game->get_id() != left_id && game->get_room_id() == room_id;
			});

		if (room_closed)
		{
			database->remove_room(room_id);
		}

		const auto room_leave_packet_bytes = worms_packet::freeze(packet_code::leave, {.value2 = room_id, .value10 = left_id});
		const auto room_close_packet_bytes = worms_packet::freeze(packet_code::close, {.value10 = room_id});

		for (const auto& user : users)
		{
			if (user->get_id() == left_id)
			{
				continue;
			}

			if (room != nullptr)
			{
				user->send_packet(room_leave_packet_bytes);
			}

			if (room_closed)
			{
				user->send_packet(room_close_packet_bytes);
			}
		}

		co_return;
	}

	static awaitable<void> disconnect_user(const std::shared_ptr<user>& client_user)
	{
		if (client_user == nullptr)
		{
			co_return;
		}

		spdlog::debug("User Session: Disconnecting user {}", client_user->get_name());

		uint32_t left_id = client_user->get_id();

		if (left_id == 0)
		{
			co_return;
		}

		uint32_t room_id = client_user->get_room_id();
		const auto database = database::get_instance();

		database->remove_user(left_id);

		// Close abandoned game
		if (const auto game = database->get_game_by_name(client_user->get_name()))
		{
			room_id = game->get_room_id();
			left_id = game->get_id();

			database->remove_game(left_id);

			const auto room_leave_packet_bytes = worms_packet::freeze(packet_code::leave, {
																  .value2 = game->get_id(),
																  .value10 = client_user->get_id()
															  });
			const auto room_close_packet_bytes = worms_packet::freeze(packet_code::close, {.value10 = game->get_id()});
			for (const auto& user : database->get_users())
			{
				if (user->get_id() == client_user->get_id())
				{
					continue;
				}

				user->send_packet(room_leave_packet_bytes);
				user->send_packet(room_close_packet_bytes);
			}
		}

		// Close abandoned room
		co_await leave_room(database->get_room(room_id), left_id);

		// Notify other users we've disconnected
		const auto packet_bytes = worms_packet::freeze(packet_code::disconnect_user, {.value10 = client_user->get_id()});
		for (const auto& user : database->get_users())
		{
			user->send_packet(packet_bytes);
		}

		co_return;
	}

	user_session::user_session(ip::tcp::socket socket): _socket(std::move(socket)), _timer(_socket.get_executor())
	{
		_timer.expires_at(std::chrono::steady_clock::time_point::max());
		server::connection_count.fetch_add(1, std::memory_order_relaxed);
	}

	user_session::~user_session()
	{
		_is_shutting_down = true;
		_socket.close();

		server::connection_count.fetch_sub(1, std::memory_order_relaxed);

		// Clear any pending packets
		net::shared_bytes bytes;
		while (_packets.try_dequeue(bytes))
		{
			// Drain the queue
		}

		spdlog::debug("User session destroyed");
	}

	awaitable<void> user_session::run()
	{
		// Keep ref alive for the full coroutine lifetime
		auto self = shared_from_this();

		co_spawn(_socket.get_executor(), [self = this]() -> awaitable<void> { co_await self->writer(); }, detached);

		_user = co_await handle_login();
		if (_user == nullptr)
		{
			spdlog::error("Failed to login");
			co_return;
		}

		spdlog::info("User {} logged in", _user->get_name());

		co_await handle_session();

		co_await disconnect_user(_user);
		co_return;
	}

	void user_session::send_packet(const std::shared_ptr<net::bytes>& packet)
	{
		_packets.enqueue(packet);
	}

	ip::address_v4 user_session::address_v4() const
	{
		return _socket.remote_endpoint().address().to_v4();
	}

	awaitable<void> user_session::writer()
	{
		using namespace std::chrono_literals;
		try
		{
			net::shared_bytes packet;
			while (_socket.is_open() && !_is_shutting_down)
			{
				// Flush everything currently queued
				while (_packets.try_dequeue(packet))
				{
					boost::system::error_code ec;
					co_await async_write(_socket, buffer(packet->data),
										 redirect_error(use_awaitable, ec));
					if (ec) co_return; // socket closed/reset
				}

				_timer.expires_after(10ms);
				boost::system::error_code ec;
				co_await _timer.async_wait(redirect_error(use_awaitable, ec));

				if (ec == error::operation_aborted) continue; // packet arrived
				if (ec) co_return; // io_context stopped
			}
		}
		catch (const std::exception& e)
		{
			spdlog::error("writer coroutine error: {}", e.what());
		}
		co_return;
	}

	awaitable<std::shared_ptr<user>> user_session::handle_login()
	{
		uint32_t user_id = 0;
		try
		{
			// Wait for the client to send a login packet
			std::vector<std::byte> incoming(1024);
			boost::system::error_code ec;

			co_await _socket.async_receive(buffer(incoming), redirect_error(use_awaitable, ec));
			if (ec)
			{
				spdlog::error("Error reading login packet: {}", ec.message());
				co_return nullptr;
			}

			auto packet_reader = net::packet_reader(incoming);
			auto login_packet = worms_packet::read_from(packet_reader);
			if (!login_packet)
			{
				spdlog::error("Error reading login packet: {}", login_packet.error());
				co_return nullptr;
			}

			const auto& optional_packet = login_packet.value();
			if (!optional_packet)
			{
				spdlog::error("Login packet is empty");
				co_return nullptr;
			}

			const auto& login_info = *optional_packet;
			if (login_info->code() != packet_code::login)
			{
				spdlog::error("Invalid packet code in login packet");
				co_return nullptr;
			}

			if (const auto& fields = login_info->fields(); !fields.value1 || !fields.value4 || !fields.name || !fields.
				session_info)
			{
				spdlog::error("Not enough data in login packet");
				co_return nullptr;
			}


			const std::string username = login_info->fields().name.value();

			// check if a username is valid and not already taken
			const auto database = database::get_instance();

			auto current_users = database->get_users();
			const auto found_user = std::ranges::find_if(current_users, [&](const auto& user)
			{
				return boost::iequals(user->get_name(), username);
			});
			if (found_user != current_users.end())
			{
				const auto bytes = worms_packet::freeze(packet_code::login_reply, {.value1 = 0, .error = 1});
				co_await _socket.async_write_some(buffer(bytes->data), use_awaitable);
				co_return nullptr;
			}
			current_users.clear();

			// create a new user and add it to the database
			user_id = database::get_next_id();
			const auto client_user = std::make_shared<user>(shared_from_this(), user_id, username,
															login_info->fields().session_info->nation);

			// Notify other users we've logged in
			const auto packet_bytes = worms_packet::freeze(packet_code::login, {
													   .value1 = user_id, .value4 = 0, .name = username,
													   .session_info = client_user->get_session_info()
												   });
			for (const auto& user : database->get_users())
			{
				user->send_packet(packet_bytes);
			}

			database->add_user(client_user);

			// Send the login reply packet
			send_packet(worms_packet::freeze(packet_code::login_reply, {.value1 = user_id, .error = 0}));

			co_return client_user;
		}
		catch (std::exception& e)
		{
			spdlog::error("Error in User Session login: {}", e.what());
			co_return database::get_instance()->get_user(user_id);
		}
	}

	awaitable<void> user_session::handle_session()
	{
		try
		{
			std::vector<std::byte> incoming(1024 * 2);
			const auto database = database::get_instance();
			net::framed_packet_reader reader;

			while (_socket.is_open())
			{
				try
				{
					boost::system::error_code ec;
					const size_t read = co_await _socket.async_receive(
						buffer(incoming),
						redirect_error(use_awaitable, ec)
					);

					if (read == 0 || ec == error::eof)
					{
						spdlog::info("User {} disconnected", _user->get_name());
						break;
					}
					if (ec)
					{
						spdlog::error("Error receiving data: {}", ec.message());
						break;
					}

					reader.append(incoming.data(), read);
					while (true)
					{
						const auto result = reader.try_read_packet();
						if (!result.has_value())
						{
							// Invalid data
							spdlog::error("Parse error: {}", result.error());
							co_return;
						}

						const auto& maybe_packet = result.value();
						if (!maybe_packet)
						{
							// Needs more data
							break;
						}

						if (!co_await packet_handler::handle_packet(_user, database, *maybe_packet))
						{
							spdlog::warn("Packet handler failed or returned false");
							co_return;
						}
					}
				}
				catch (const std::exception& e)
				{
					spdlog::error("Error handling packet: {}", e.what());
					// Continue instead of breaking to be more resilient
					continue;
				}
			}
		}
		catch (const std::exception& e)
		{
			spdlog::error("Fatal error in User Session: {}", e.what());
		}

		// Ensure the socket is closed
		try
		{
			if (_socket.is_open())
			{
				_socket.close();
			}
		}
		catch (const std::exception& e)
		{
			spdlog::error("Error closing socket: {}", e.what());
		}

		co_return;
	}
}
