#include <iostream>
#include <memory>
#include <string>

#include <vector>
#include <ranges>

#include "header_only/packet_buffers.hpp"


#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/cfg/env.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <boost/asio.hpp>

#include "user_session.hpp"

using boost::asio::awaitable;
using boost::asio::use_awaitable;
using namespace boost::asio;

awaitable<void> listener(uint16_t port, size_t max_connections)
{
	spdlog::info("Starting server listener");
	try
	{
		auto executor = co_await this_coro::executor;
		ip::tcp::acceptor acceptor(executor, {ip::tcp::v4(), port});

		if (acceptor.is_open())
		{
			spdlog::info("Listening on port {}", port);
			acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
		}
		else
		{
			spdlog::error("Error opening port {}", port);
			co_return;
		}

		for (;;)
		{
			boost::system::error_code ec;
			ip::tcp::socket socket{executor};

			spdlog::debug("Waiting for connection");
			co_await acceptor.async_accept(socket, redirect_error(use_awaitable, ec));

			if (!ec)
			{
				const auto remote_endpoint = socket.remote_endpoint();
				spdlog::info("Accepted connection from {}", remote_endpoint.address().to_string());

				if (worms_server::user_session::connection_count.load(std::memory_order_acquire) >= max_connections)
				{
					spdlog::error("Too many connections ({}), closing connection",
								  worms_server::user_session::connection_count.load());
					socket.close();
					continue;
				}


				socket.set_option(ip::tcp::no_delay(true));
				socket.set_option(ip::tcp::socket::keep_alive(true));

				const auto session = std::make_shared<worms_server::user_session>(std::move(socket));

				spdlog::debug("Spawning session coroutine");
				co_spawn(executor, session->run(),detached);
			}
			else
			{
				spdlog::error("Error accepting connection: {}", ec.message());
			}
		}
	}
	catch (const std::exception& e)
	{
		spdlog::error("Fatal listener error: {}", e.what());
	}
	catch (...)
	{
		spdlog::error("Unknown fatal listener error");
	}

	spdlog::info("Listener shutting down");
	co_return;
}


int main(const int argc, char** argv)
{
	uint16_t port = 17000;
	size_t max_connections = 1000;
	size_t max_threads = std::thread::hardware_concurrency();

	spdlog::init_thread_pool(8192, 1); // queue size, number of threads
	const auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	const auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/worms_server.log", 0, 0, true);

	std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
	const auto logger = std::make_shared<spdlog::async_logger>(
		"async_logger",
		sinks.begin(),
		sinks.end(),
		spdlog::thread_pool(), // use global thread pool
		spdlog::async_overflow_policy::block);

	spdlog::set_level(spdlog::level::info);
	spdlog::cfg::load_env_levels();

	spdlog::apply_logger_env_levels(logger);

	spdlog::set_default_logger(logger);
	spdlog::flush_on(spdlog::level::err);

	// Make sure to flush when the program exits
	std::atexit([]()
	{
		spdlog::shutdown();
	});


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

		spdlog::info("Press Ctrl+C to exit");

		// Multithreaded run
		std::vector<std::jthread> threads;
		for (size_t i = 0; i < (max_threads - 1); ++i)
		{
			threads.emplace_back([&io_context]
			{
				io_context.run();
			});
		}

		// Run on the main thread too
		io_context.run();
	}
	catch (const std::exception& e)
	{
		spdlog::error("Fatal: {}", e.what());
	}

	return 0;
}
