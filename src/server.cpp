//
// Created by blomq on 2025-07-14.
//

#include "server.hpp"

#include "user_session.hpp"
#include "spdlog/spdlog.h"


namespace worms_server
{
	std::atomic<unsigned int> server::connection_count{0};

	server::server(const uint16_t port, const size_t max_connections)
		: _port(port),
		  _max_connections(max_connections),
		  _signals(_io_context, SIGINT, SIGTERM)
	{
		_signals.async_wait([this](const boost::system::error_code&, int)
		{
			stop();
		});
	}

	awaitable<void> server::listener()
	{
		auto executor = co_await this_coro::executor;
		ip::tcp::acceptor acceptor(executor, {ip::tcp::v4(), _port});
		acceptor.set_option(ip::tcp::acceptor::reuse_address(true));

		spdlog::info("Listening on port {}", _port);

		for (;;)
		{
			ip::tcp::socket socket(executor);
			boost::system::error_code ec;

			co_await acceptor.async_accept(socket, redirect_error(use_awaitable, ec));

			if (!ec)
			{
				if (connection_count.load(std::memory_order_acquire) >= _max_connections)
				{
					spdlog::warn("Too many connections, refusing client");
					socket.close();
					continue;
				}

				socket.set_option(ip::tcp::no_delay(true));
				socket.set_option(ip::tcp::socket::keep_alive(true));

				const auto session = std::make_shared<worms_server::user_session>(std::move(socket));
				co_spawn(executor, session->run(), detached);
			}
			else
			{
				spdlog::error("Accept error: {}", ec.message());
			}
		}
	}

	void server::run(const size_t thread_count)
	{
		co_spawn(_io_context, listener(), detached);
		spdlog::info("Press Ctrl+C to exit");

		for (size_t i = 0; i < thread_count - 1; ++i)
		{
			_threads.emplace_back([this]()
			{
				_io_context.run();
			});
		}

		_io_context.run();
	}

	void server::stop()
	{
		_io_context.stop();
	}
}
