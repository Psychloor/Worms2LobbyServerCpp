#include <iostream>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <optional>
#include <algorithm>
#include <ranges>

#include "header_only/packet_buffers.hpp"

#include "database.hpp"
#include "packet_code.hpp"
#include "user.hpp"
#include "worms_packet.hpp"

using tcp = boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::use_awaitable;
using namespace boost::asio;

std::atomic<size_t> connection_count{0};

awaitable<std::shared_ptr<worms_server::user>> try_to_login(ip::tcp::socket socket)
{
	// Wait for the client to send a login packet
	uint32_t user_id = 0;
	std::vector<std::byte> incoming(100);
	boost::system::error_code error_code;

	co_await socket.async_receive(buffer(incoming), redirect_error(use_awaitable, error_code));
	if (error_code != boost::system::errc::success)
	{
		std::cerr << "Error reading login packet: " << error_code.value() << "\n";
		co_return nullptr;
	}

	auto packet_reader = net::packet_reader(incoming);
	const auto login_packet = worms_server::worms_packet::read_from(packet_reader);
	if (!login_packet.has_value())
	{
		std::cerr << "Error reading login packet: " << login_packet.error() << "\n";
		co_return nullptr;
	}

	if (login_packet->value()->code() != worms_server::packet_code::login)
	{
		std::cerr << "Invalid packet code in login packet\n";
		co_return nullptr;
	}

	const auto login_info = login_packet.value().value();
	if (!login_info->get_value1().has_value() || !login_info->get_value4().has_value() || !login_info->get_name().
		has_value() || login_info->get_session_info().has_value() || login_info->get_name().value().size() >= 3)
	{
		std::cerr << "Not enough data in login packet\n";
		co_return nullptr;
	}

	const std::string username = login_info->get_name().value();

	// check if a username is valid and not already taken
	const auto database = worms_server::database::get_instance();

	auto current_users = database->get_users();
	const auto found_user = std::ranges::find_if(current_users, [&](const auto& user) { return boost::iequals(user->get_name(), username); });
	if (found_user != current_users.end())
	{
		auto packet = worms_server::worms_packet(worms_server::packet_code::login_reply).with_value1(0).
			with_error(1);
		auto writer = net::packet_writer();
		packet.write_to(writer);
		socket.async_write_some(buffer(writer.span()), use_awaitable);
		co_return nullptr;
	}
	current_users.clear();

	// create a new user and add it to the database
	user_id = database->get_next_id();
	const auto client_user = std::make_shared<worms_server::user>(std::move(socket), user_id, username,
																  login_info->get_session_info()->nation);

	// Notify other users we've logged in
	auto packet = worms_server::worms_packet(worms_server::packet_code::login).with_value1(user_id).with_value4(0).
		with_name(username).with_session_info(client_user->get_session_info());
	auto writer = net::packet_writer();
	packet.write_to(writer);

	auto packet_bytes = writer.span();
	for (const auto& user : database->get_users())
	{
		user->send_packet(packet_bytes);
	}

	database->add_user(client_user);
	client_user->start_writer();

	// Send the login reply packet
	writer.clear();
	packet = worms_server::worms_packet(worms_server::packet_code::login_reply).with_value1(user_id).with_error(0);
	packet.write_to(writer);
	client_user->send_packet(writer.span());

	co_return client_user;
}

awaitable<void> session(ip::tcp::socket socket)
{
	uint32_t user_id = 0;
	try
	{
		const auto client_user = co_await try_to_login(std::move(socket));
		if (client_user == nullptr)
		{
			std::cerr << "Login failed\n";
			co_return;
		}

		user_id = client_user->get_id();

		boost::system::error_code error_code;
		std::vector<std::byte> incoming(1024);

		bool exit_requested = false;
		for (;;)
		{

			// Wait for the client to send a packet
			const size_t read = co_await client_user->async_receive(buffer(incoming),  error_code);
			if (error_code != boost::system::errc::success)
			{
				std::cerr << "Error reading packet: " << error_code.value() << "\n";
				break;
			}

			// Keep reading packets until we can't
			for (;;)
			{
				auto packet_reader = net::packet_reader(incoming);

				// ReSharper disable once CppDeclarationHidesLocal
				const auto packet = worms_server::worms_packet::read_from(packet_reader);
				if (!packet.has_value())
				{
					std::cerr << "Error reading packet: " << packet.error() << "\n";
					exit_requested = true;
					break;
				}

				const auto packet_result = packet.value();
				if (!packet_result.has_value())
				{
					// Needs more data
					break;
				}

				incoming.erase(incoming.begin(), incoming.begin() + packet_reader.bytes_read());

				const auto worms_packet = packet_result.value();
				const auto packet_code = worms_packet->code();

				// TODO: handle packets here
			}

			if (exit_requested)
			{
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
		const auto database = worms_server::database::get_instance();
		database->remove_user(user_id);
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
	}

	try
	{
		io_context io_context(2);

		co_spawn(io_context, listener(port, max_connections), detached);

		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&](const boost::system::error_code& error, int signal_number)
		{
			io_context.stop();
		});

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
