#include <iostream>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <coroutine>

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <optional>
#include <algorithm>
#include <ranges>

#include "header_only/packet_buffers.hpp"

#include "database.hpp"
#include "packet_code.hpp"
#include "game.hpp"
#include "room.hpp"
#include "user.hpp"
#include "worms_packet.hpp"

using tcp = boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::use_awaitable;
using namespace boost::asio;
using namespace std::chrono_literals;

using namespace worms_server;

std::atomic<size_t> connection_count{0};

awaitable<bool> on_chat_room(const std::shared_ptr<user>& client_user,
							 const std::shared_ptr<database>& database,
							 const std::shared_ptr<worms_packet>& packet)
{
	if (packet->fields().value0.value_or(0) != client_user->get_id() || !packet->fields().value3.has_value() || !packet
		->
		fields().data.has_value())
	{
		std::cerr << "Invalid packet data\n";
		co_return false;
	}

	const auto& target_id = packet->fields().value3.value();
	const auto& message = packet->fields().data.value();
	const auto client_id = client_user->get_id();

	if (message.starts_with(std::format("GRP:[ {} ]  ", client_user->get_name())))
	{
		// Check if the user can access the room.
		if (client_user->get_room_id() == target_id)
		{
			net::packet_writer writer;

			// Notify all users of the room.
			worms_packet(packet_code::chat_room, {.value0 = client_id, .value3 = target_id, .data = message}).
				write_to(writer);
			const auto packet_bytes = writer.span();

			for (const auto& user : database->get_users())
			{
				if (user->get_room_id() == target_id && user->get_id() != client_id)
				{
					user->send_packet(packet_bytes);
				}
			}

			// Notify sender
			writer.clear();
			worms_packet(packet_code::chat_room_reply, {.error = 0}).write_to(writer);
			client_user->send_packet(writer.span());
			co_return true;
		}

		// Notify sender
		net::packet_writer writer;
		worms_packet(packet_code::chat_room_reply, {.error = 1}).write_to(writer);
		client_user->send_packet(writer.span());
		co_return true;
	}

	if (message.starts_with(std::format("PRV:[ {} ]  ", client_user->get_name())))
	{
		const auto& target_user = database->get_user(target_id);
		if (target_user == nullptr)
		{
			net::packet_writer writer;
			worms_packet(packet_code::chat_room_reply, {.error = 1}).write_to(writer);
			client_user->send_packet(writer.span());
			co_return true;
		}

		// Notify Target
		net::packet_writer writer;
		worms_packet(packet_code::chat_room, {.value0 = client_id, .value3 = target_id, .data = message}).
			write_to(writer);
		target_user->send_packet(writer.span());
		writer.clear();

		// Notify Sender
		worms_packet(packet_code::chat_room_reply, {.error = 1}).write_to(writer);
		client_user->send_packet(writer.span());
		co_return true;
	}

	co_return true;
}

awaitable<void> leave_room(const std::shared_ptr<room>& room, const uint32_t left_id)
{
	const auto database = database::get_instance();
	const auto users = database->get_users();
	const uint32_t room_id = room == nullptr ? 5 : room->get_id();

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
		std::cout << "Room " << room_id << " closed\n";
	}

	net::packet_writer writer;
	worms_packet(packet_code::leave, {.value2 = room_id, .value10 = left_id}).write_to(writer);
	const auto room_leave_packet_bytes = writer.span();

	writer.clear();
	worms_packet(packet_code::close, {.value10 = room_id}).write_to(writer);
	const auto room_close_packet_bytes = writer.span();
	writer.clear();

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

awaitable<bool> on_list_rooms(const std::shared_ptr<user>& client_user,
							  const std::shared_ptr<database>& database,
							  const std::shared_ptr<worms_packet>& packet)
{
	if (packet->fields().value4.value_or(0) != 0)
	{
		std::cerr << "Invalid packet data\n";
		co_return false;
	}

	const auto rooms = database->get_rooms();
	net::packet_writer writer;
	for (const auto& room : rooms)
	{
		worms_packet(packet_code::list_item, {
						 .value1 = room->get_id(), .name = std::string(room->get_name()),
						 .data = "",
						 .session_info = room->get_session_info()
					 }).write_to(writer);


		const auto packet_bytes = writer.span();
		writer.clear();
		client_user->send_packet(packet_bytes);
	}

	worms_packet(packet_code::list_end).write_to(writer);
	const auto packet_bytes = writer.span();
	client_user->send_packet(packet_bytes);

	co_return true;
}

awaitable<bool> on_list_users(const std::shared_ptr<user>& client_user,
							  const std::shared_ptr<database>& database,
							  const std::shared_ptr<worms_packet>& packet)
{
	if (packet->fields().value4.value_or(0) != 0 || packet->fields().value2.value_or(0) != client_user->get_room_id())
	{
		std::cerr << "Invalid packet data\n";
		co_return false;
	}

	const auto users = database->get_users();
	const auto room_id = client_user->get_room_id();
	net::packet_writer writer;
	for (const auto& user : users)
	{
		if (user->get_room_id() != room_id)
		{
			continue;
		}

		worms_packet(packet_code::list_item, {
						 .value1 = user->get_id(), .name = std::string(user->get_name()), .data = "",
						 .session_info = user->get_session_info()
					 }).write_to(writer);
		const auto packet_bytes = writer.span();
		writer.clear();
		client_user->send_packet(packet_bytes);
	}

	worms_packet(packet_code::list_end).write_to(writer);
	const auto packet_bytes = writer.span();
	client_user->send_packet(packet_bytes);

	co_return true;
}

awaitable<bool> on_list_games(const std::shared_ptr<user>& client_user,
							  const std::shared_ptr<database>& database,
							  const std::shared_ptr<worms_packet>& packet)
{
	if (packet->fields().value4.value_or(0) != 0 || packet->fields().value2.value_or(0) != client_user->get_room_id())
	{
		std::cerr << "Invalid packet data\n";
		co_return false;
	}

	const auto games = database->get_games();
	net::packet_writer writer;

	for (const auto& game : games)
	{
		if (game->get_room_id() != client_user->get_room_id())
		{
			continue;
		}

		worms_packet(packet_code::list_item, {
						 .value1 = game->get_id(),
						 .name = std::string(game->get_name()), .data = game->get_address().to_string(),
						 .session_info = game->get_session_info()
					 }).write_to(writer);
		const auto packet_bytes = writer.span();
		writer.clear();
		client_user->send_packet(packet_bytes);
	}

	worms_packet(packet_code::list_end).write_to(writer);
	const auto packet_bytes = writer.span();
	client_user->send_packet(packet_bytes);

	co_return true;
}

awaitable<bool> on_create_room(const std::shared_ptr<user>& client_user,
							   const std::shared_ptr<database>& database,
							   const std::shared_ptr<worms_packet>& packet)
{
	if (packet->fields().value1.value_or(0) != 0 || packet->fields().value4.value_or(0) != 0 || packet->fields().data.
		value_or("").empty() || packet->fields().name.value_or("").empty() || !packet->fields().session_info.
		has_value())
	{
		std::cerr << "Invalid packet data\n";
		co_return false;
	}

	// Check if the room name is valid is not already taken.
	if (std::ranges::any_of(database->get_rooms(),
							[client_name = client_user->get_name()](const auto& room) -> bool
							{
								return boost::iequals(room->get_name(), client_name);
							}))
	{
		net::packet_writer writer;
		worms_packet(packet_code::create_room_reply, {.value1 = 0, .error = 1}).write_to(writer);
		const auto packet_bytes = writer.span();
		client_user->send_packet(packet_bytes);

		co_return false;
	}

	const auto room_id = database::get_next_id();
	const auto room = std::make_shared<worms_server::room>(room_id, packet->fields().name.value(),
														   packet->fields().session_info->nation,
														   client_user->get_address());
	database::get_instance()->add_room(room);


	net::packet_writer writer;
	worms_packet(packet_code::create_room, {
					 .value1 = room_id, .value4 = 0, .name = std::string(room->get_name()), .data = "",
					 .session_info = room->get_session_info()
				 }).write_to(writer);
	const auto room_packet_bytes = writer.span();

	// notify others
	for (const auto& user : database->get_users())
	{
		if (user->get_id() == client_user->get_id()) { continue; }

		user->send_packet(room_packet_bytes);
	}

	// Send the creation room reply packet
	writer.clear();
	worms_packet(packet_code::create_room_reply, {.value1 = room_id, .error = 0}).write_to(writer);
	const auto packet_bytes = writer.span();
	client_user->send_packet(packet_bytes);

	co_return true;
}

awaitable<bool> on_join(const std::shared_ptr<user>& client_user,
						const std::shared_ptr<database>& database,
						const std::shared_ptr<worms_packet>& packet)
{
	if (!packet->fields().value2.has_value() || packet->fields().value10.value_or(0) != client_user->get_id())
	{
		std::cerr << "Invalid packet data\n";
		co_return false;
	}

	// Require a valid room or game ID.
	if (std::ranges::any_of(database->get_rooms(), [join_id = packet->fields().value2.value()](const auto& room) -> bool
	{
		return room->get_id() == join_id;
	}))
	{
		client_user->set_room_id(packet->fields().value2.value());

		// Notify other users about the join.
		net::packet_writer writer;
		worms_packet(packet_code::join, {
						 .value2 = packet->fields().value2.value(), .value10 = client_user->get_id()
					 }).write_to(writer);
		const auto packet_bytes = writer.span();
		for (const auto& user : database->get_users())
		{
			if (user->get_id() == client_user->get_id()) { continue; }
			user->send_packet(packet_bytes);
		}

		writer.clear();
		worms_packet(packet_code::join_reply, {.error = 0}).write_to(writer);
		const auto packet_bytes2 = writer.span();
		client_user->send_packet(packet_bytes2);

		co_return true;
	}

	if (std::ranges::any_of(database->get_games(),
							[join_id = packet->fields().value2.value(), room_id = client_user->get_room_id()](
							const auto& game) -> bool
							{
								return game->get_id() == join_id && game->get_room_id() == room_id;
							}))
	{
		// Notify other users about the join.
		net::packet_writer writer;
		worms_packet(packet_code::join, {
						 .value2 = client_user->get_room_id(), .value10 = client_user->get_id()
					 }).write_to(writer);
		const auto packet_bytes = writer.span();
		for (const auto& user : database->get_users())
		{
			if (user->get_id() == client_user->get_id()) { continue; }
			user->send_packet(packet_bytes);
		}

		writer.clear();
		worms_packet(packet_code::join_reply, {.error = 0}).write_to(writer);
		const auto packet_bytes2 = writer.span();
		client_user->send_packet(packet_bytes2);

		co_return true;
	}

	net::packet_writer writer;
	worms_packet(packet_code::join_reply, {.error = 1}).write_to(writer);
	const auto packet_bytes = writer.span();
	client_user->send_packet(packet_bytes);

	co_return true;
}

awaitable<bool> on_leave(const std::shared_ptr<user>& client_user,
						 const std::shared_ptr<database>& database,
						 const std::shared_ptr<worms_packet>& packet)
{
	if (packet->fields().value10.value_or(0) != client_user->get_id() || !packet->fields().value2.has_value())
	{
		std::cerr << "Invalid packet data\n";
		co_return false;
	}

	// Require valid room ID (never sent for games, users disconnect if leaving a game).
	if (packet->fields().value2.value() == client_user->get_room_id())
	{
		co_await leave_room(database->get_room(client_user->get_room_id()), client_user->get_id());
		client_user->set_room_id(0);

		// Reply to leaver.
		net::packet_writer writer;
		worms_packet(packet_code::leave_reply, {.error = 0}).write_to(writer);
		const auto packet_bytes = writer.span();
		client_user->send_packet(packet_bytes);

		co_return true;
	}

	// Reply to leaver. (failed to find)
	net::packet_writer writer;
	worms_packet(packet_code::leave_reply, {.error = 1}).write_to(writer);
	const auto packet_bytes = writer.span();
	client_user->send_packet(packet_bytes);

	co_return true;
}

awaitable<bool> on_close(const std::shared_ptr<user>& client_user,
						 const std::shared_ptr<database>& database,
						 const std::shared_ptr<worms_packet>& packet)
{
	if (!packet->fields().value10.has_value())
	{
		std::cerr << "Invalid packet data\n";
		co_return false;
	}

	// Never sent for games, users disconnect if leaving a game.
	// Reply success to the client, the server decides when to actually close rooms.
	net::packet_writer writer;
	worms_packet(packet_code::close_reply, {.error = 0}).write_to(writer);
	const auto packet_bytes = writer.span();
	client_user->send_packet(packet_bytes);

	co_return true;
}

awaitable<bool> on_create_game(const std::shared_ptr<user>& client_user,
							   const std::shared_ptr<database>& database,
							   const std::shared_ptr<worms_packet>& packet)
{
	if (packet->fields().value1.value_or(1) != 0
		|| packet->fields().value2.value_or(0) != client_user->get_room_id()
		|| packet->fields().value4.value_or(0) != 0x800
		|| !packet->fields().data.has_value()
		|| !packet->fields().name.has_value()
		|| !packet->fields().session_info.has_value())
	{
		std::cerr << "Invalid packet data\n";
		co_return false;
	}

	// Require valid room ID and IP.
	ip::address parsed_ip;
	try
	{
		parsed_ip = ip::make_address(packet->fields().data.value());
	}
	catch (const boost::system::error_code& e)
	{
		std::cerr << "Invalid IP address: " << e.what() << "\n";
		co_return false;
	}

	if (parsed_ip.is_v4() && client_user->get_address() == parsed_ip)
	{
		// Create a new game.
		const auto game_id = database::get_next_id();
		const auto game = std::make_shared<worms_server::game>(game_id, client_user->get_name(),
															   client_user->get_session_info().nation,
															   client_user->get_room_id(), client_user->get_address(),
															   packet->fields().session_info.value().access);
		database->add_game(game);

		// Notify other users about the new game, even those in other rooms.
		net::packet_writer writer;
		worms_packet(packet_code::create_game, {
						 .value1 = game_id,
						 .value2 = game->get_room_id(),
						 .value4 = 0x800,
						 .name = std::string(game->get_name()),
						 .data = game->get_address().to_string(),
						 .session_info = game->get_session_info()
					 }).write_to(writer);

		const auto packet_bytes = writer.span();
		for (const auto& user : database->get_users())
		{
			if (user->get_id() == client_user->get_id()) { continue; }
			user->send_packet(packet_bytes);
		}

		// Send reply to host
		writer.clear();
		worms_packet(packet_code::create_game_reply, {.value1 = game_id, .error = 0}).write_to(writer);
		const auto packet_bytes2 = writer.span();
		client_user->send_packet(packet_bytes2);
	}

	net::packet_writer writer;
	worms_packet(packet_code::create_game_reply, {.value1 = 0, .error = 2}).write_to(writer);
	const auto packet_bytes = writer.span();
	client_user->send_packet(packet_bytes);

	writer.clear();
	worms_packet(packet_code::chat_room, {
					 .value0 = client_user->get_id(), .value3 = client_user->get_room_id(),
					 .data =
					 "GRP:Cannot host your game. Please use FrontendKitWS with fkNetcode. More information at worms2d.info/fkNetcode"
				 }).write_to(writer);
	const auto packet_bytes2 = writer.span();
	client_user->send_packet(packet_bytes2);

	co_return true;
}

awaitable<bool> on_connect_game(const std::shared_ptr<user>& client_user,
								const std::shared_ptr<database>& database,
								const std::shared_ptr<worms_packet>& packet)
{
	if (!packet->fields().value0.has_value())
	{
		std::cerr << "Invalid packet data\n";
		co_return false;
	}

	// Require valid game ID and user to be in appropriate room.
	const auto games = database->get_games();
	const auto it = std::ranges::find_if(
		games, [game_id = packet->fields().value0.value(), room_id = client_user->get_room_id()](
		const auto& game) -> bool
		{
			return game->get_id() == game_id && game->get_room_id() == room_id;
		});

	if (it == games.end())
	{
		net::packet_writer writer;
		worms_packet(packet_code::connect_game_reply, {.data = "", .error = 1}).write_to(writer);
		const auto packet_bytes = writer.span();
		client_user->send_packet(packet_bytes);
	}
	else
	{
		net::packet_writer writer;
		worms_packet(packet_code::connect_game_reply, {.data = (*it)->get_address().to_string(), .error = 0}).
			write_to(writer);
		const auto packet_bytes = writer.span();
		client_user->send_packet(packet_bytes);
	}

	co_return true;
}

awaitable<void> disconnect_user(const std::shared_ptr<user>& client_user)
{
	if (client_user == nullptr)
	{
		co_return;
	}

	uint32_t left_id = client_user->get_id();
	uint32_t room_id = client_user->get_room_id();
	const auto database = database::get_instance();

	database->remove_user(left_id);

	// Close abandoned game
	const auto game = database->get_game_by_name(client_user->get_name());
	if (game != nullptr)
	{
		room_id = game->get_room_id();
		left_id = game->get_id();

		database->remove_game(left_id);
		std::cout << "Game " << left_id << " closed\n";

		net::packet_writer writer;

		worms_packet(packet_code::leave, {.value2 = game->get_id(), .value10 = client_user->get_id()}).write_to(writer);
		const auto room_leave_packet_bytes = writer.span();
		writer.clear();

		worms_packet(packet_code::close, {.value10 = game->get_id()}).write_to(writer);
		const auto room_close_packet_bytes = writer.span();
		writer.clear();


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
	net::packet_writer writer;
	worms_packet(packet_code::disconnect_user, {.value10 = client_user->get_id()}).write_to(writer);
	const auto packet_bytes = writer.span();
	for (const auto& user : database->get_users())
	{
		user->send_packet(packet_bytes);
	}

	co_return;
}

awaitable<std::shared_ptr<user>> try_to_login(ip::tcp::socket socket)
{
	// Wait for the client to send a login packet
	uint32_t user_id = 0;
	std::vector<std::byte> incoming(1024);
	incoming.reserve(1024);
	boost::system::error_code error_code;

	co_await socket.async_receive(buffer(incoming), redirect_error(use_awaitable, error_code));
	if (error_code != boost::system::errc::success)
	{
		std::cerr << "Error reading login packet: " << error_code.value() << "\n";
		co_return nullptr;
	}

	auto packet_reader = net::packet_reader(incoming);
	const auto login_packet = worms_packet::read_from(packet_reader);
	if (!login_packet.has_value())
	{
		std::cerr << "Error reading login packet: " << login_packet.error() << "\n";
		co_return nullptr;
	}

	if (login_packet->value()->code() != packet_code::login)
	{
		std::cerr << "Invalid packet code in login packet\n";
		co_return nullptr;
	}

	auto login_info = *login_packet.value();
	if (!login_info->fields().value1.has_value() || !login_info->fields().value4.has_value() || !login_info->fields().
		name.
		has_value() || !login_info->fields().session_info.has_value())
	{
		std::cerr << "Not enough data in login packet\n";

		std::cerr << "value1: " << login_info->fields().value1.value_or(8) << "\n";
		std::cerr << "value4: " << login_info->fields().value4.value_or(8) << "\n";
		std::cerr << "name: " << login_info->fields().name.value_or("") << "\n";

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
		auto writer = net::packet_writer();
		worms_packet(packet_code::login_reply, {.value1 = 0, .error = 1}).write_to(writer);
		co_await socket.async_write_some(buffer(writer.span()), use_awaitable);
		co_return nullptr;
	}
	current_users.clear();

	// create a new user and add it to the database
	user_id = database::get_next_id();
	const auto client_user = std::make_shared<user>(std::move(socket), user_id, username,
													login_info->fields().session_info->nation);
	client_user->start_writer();

	// Notify other users we've logged in
	auto writer = net::packet_writer();
	worms_packet(packet_code::login, {
					 .value1 = user_id, .value4 = 0, .name = username, .session_info = client_user->get_session_info()
				 }).write_to(writer);

	auto packet_bytes = writer.span();
	for (const auto& user : database->get_users())
	{
		user->send_packet(packet_bytes);
	}

	database->add_user(client_user);

	// Send the login reply packet
	writer.clear();
	worms_packet(packet_code::login_reply, {.value1 = user_id, .error = 0}).write_to(writer);
	client_user->send_packet(writer.span());

	co_return client_user;
}

awaitable<void> session(ip::tcp::socket socket)
{
	uint32_t user_id = 0;

	try
	{
		bool alive = true;
		bool logged_in = false;

		const auto client_user = co_await try_to_login(std::move(socket));


		if (client_user == nullptr)
		{
			std::cerr << "Login failed\n";
			connection_count.fetch_sub(1, std::memory_order_relaxed);
			co_return;
		}

		logged_in = true;
		alive = true;

		std::cout << "Logged in as " << client_user->get_name() << "\n";

		user_id = client_user->get_id();

		boost::system::error_code error_code;
		std::vector<std::byte> incoming(1024 * 2);
		auto database = database::get_instance();

		while (alive)
		{
			// Wait for the client to send a packet
			const size_t read = co_await client_user->async_receive(buffer(incoming), error_code);

			if (error_code != boost::system::errc::success)
			{
				std::cerr << "Error reading packet: " << error_code.value() << "\n";
				break;
			}

			if (read == 0)
			{
				std::cerr << "Client disconnected\n";
				break;
			}

			auto packet_reader = net::packet_reader(incoming);
			const auto packet = worms_packet::read_from(packet_reader);
			if (!packet.has_value())
			{
				std::cerr << "Error reading packet: " << packet.error() << "\n";
				alive = false;
				break;
			}

			const auto& optional_packet = packet.value();
			if (!optional_packet.has_value())
			{
				// Needs more data
				continue;
			}

			if (const auto consumed = packet_reader.bytes_read();
				packet_reader.bytes_read() > 0 && consumed < incoming.size())
			{
				std::copy(
					incoming.begin() + static_cast<ptrdiff_t>(consumed),
					incoming.end(),
					incoming.begin()
				);
			}

			const auto& parsed_packet = optional_packet.value();

			switch (const auto packet_code = parsed_packet->code())
			{
			case packet_code::login:
				std::cerr << "Login packet shouldn't be received again\n";
				break;

			case packet_code::chat_room:
				if (!co_await on_chat_room(client_user, database, parsed_packet))
				{
					alive = false;
				}
				break;

			case packet_code::list_users:
				if (!co_await on_list_users(client_user, database, parsed_packet))
					alive = false;
				break;

			case packet_code::list_rooms:
				if (!co_await on_list_rooms(client_user, database, parsed_packet))
					alive = false;
				break;

			case packet_code::list_games:
				if (!co_await on_list_games(client_user, database, parsed_packet))
					alive = false;
				break;

			case packet_code::create_room:
				if (!co_await on_create_room(client_user, database, parsed_packet))
					alive = false;
				break;

			case packet_code::join:
				if (!co_await on_join(client_user, database, parsed_packet))
					alive = false;
				break;

			case packet_code::leave:
				if (!co_await on_leave(client_user, database, parsed_packet))
					alive = false;
				break;

			case packet_code::create_game:
				if (!co_await on_create_game(client_user, database, parsed_packet))
					alive = false;
				break;

			case packet_code::connect_game:
				if (!co_await on_connect_game(client_user, database, parsed_packet))
					alive = false;
				break;

			case packet_code::close:
				if (!co_await on_close(client_user, database, parsed_packet))
					alive = false;
				break;

			default:
				std::cerr << "Unknown packet code: " << static_cast<uint32_t>(packet_code) << "\n";
				break;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error in session: " << e.what() << "\n";
	}

	if (user_id != 0)
	{
		// Remove the user from the database
		disconnect_user(database::get_instance()->get_user(user_id));
		std::cout << "User " << user_id << " disconnected\n";
	}

	connection_count.fetch_sub(1, std::memory_order_relaxed);
	co_return;
}


awaitable<void> listener(const uint16_t port, const size_t max_connections)
{
	boost::system::error_code ec;
	auto executor = co_await this_coro::executor;
	ip::tcp::acceptor acceptor(executor, {ip::tcp::v4(), port});
	std::cout << "Listening on port " << port << "\n";

	acceptor.set_option(ip::tcp::acceptor::reuse_address(true));


	for (;;)
	{
		ip::tcp::socket socket{executor};
		co_await acceptor.async_accept(socket, redirect_error(use_awaitable, ec));
		if (ec == boost::system::errc::success)
		{
			std::cout << "Accepted connection from " << socket.remote_endpoint().address().to_string() << "\n";

			if (connection_count.load(std::memory_order_relaxed) >= max_connections)
			{
				std::cout << "Too many connections, closing connection\n";
				socket.close();
				continue;
			}

			connection_count.fetch_add(1, std::memory_order_relaxed);

			socket.set_option(ip::tcp::no_delay(true));
			socket.set_option(ip::tcp::socket::keep_alive(true));

			co_spawn(executor, session(std::move(socket)), detached);
		}
		else
		{
			std::cerr << "Error accepting connection: " << ec << "\n";
		}
	}
}


int main(const int argc, char** argv)
{
	uint16_t port = 17000;
	size_t max_connections = 1000;
	size_t max_threads = std::thread::hardware_concurrency();

	auto args = std::vector<std::string>(argv, argv + argc);
	for (const auto args_slide = std::ranges::slide_view(args, 2); const auto& arg : args_slide)
	{
		if (arg[0] == "-p" || arg[0] == "--port")
		{
			port = std::stoi(arg[1]);
			if (port < 1 || port > 65535)
			{
				std::cerr << "Invalid port number, defaulting to 17000\n";
				port = 17000;
			}
		}

		if (arg[0] == "-c" || arg[0] == "--connections")
		{
			max_connections = std::stoi(arg[1]);
			if (max_connections < 1)
			{
				std::cerr << "Invalid connection count, defaulting to 1000\n";
				max_connections = 1000;
			}
		}

		if (arg[0] == "-t" || arg[0] == "--threads")
		{
			max_threads = std::stoi(arg[1]);
			if (max_threads < 1)
			{
				std::cerr << "Invalid thread count, defaulting to " << std::thread::hardware_concurrency() << "\n";
				max_threads = std::thread::hardware_concurrency();
			}
			else if (max_threads > std::thread::hardware_concurrency())
			{
				std::cerr << "Thread count cannot be higher than the number of cores, defaulting to " <<
					std::thread::hardware_concurrency()
					<< "\n";
				max_threads = std::thread::hardware_concurrency();
			}
		}
	}

	try
	{
		io_context io_context;

		co_spawn(io_context, listener(port, max_connections), detached);

		signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&](const boost::system::error_code& error, int signal_number)
		{
			io_context.stop();
		});

		std::cout << "Press Ctrl+C to exit\n";

		// Multithreaded run
		if (max_threads > 1)
		{
			std::vector<std::jthread> threads;
			for (size_t i = 0; i < max_threads; ++i)
			{
				threads.emplace_back([&io_context]
				{
					io_context.run();
				});
			}
		}

		// Run on the main thread too
		io_context.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Fatal: " << e.what() << "\n";
	}

	std::cout << "Exiting...\n";
	std::string input;
	std::cin >> input;

	return 0;
}
