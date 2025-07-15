//
// Created by blomq on 2025-07-14.
//

#include "server.hpp"

#include "spdlog/spdlog.h"
#include "user_session.hpp"


namespace worms_server
{
    std::atomic<unsigned int> server::connection_count{0};

    server::server(const uint16_t port, const size_t max_connections)
    : port_(port), max_connections_(max_connections),
      signals_(io_context_, SIGINT, SIGTERM)
    {
        signals_.async_wait([this](const boost::system::error_code&, int)
        {
            stop();
        });
    }

    awaitable<void> server::listener()
    {
        auto executor = co_await this_coro::executor;
        ip::tcp::acceptor acceptor(executor, {ip::tcp::v4(), port_});
        acceptor.set_option(ip::tcp::acceptor::reuse_address(true));

        spdlog::info("Listening on port {}", port_);

        for (;;)
        {
            ip::tcp::socket socket(executor);
            boost::system::error_code ec;

            co_await acceptor.async_accept(socket,
                redirect_error(use_awaitable, ec));

            if (!ec)
            {
                if (connection_count.load(std::memory_order_acquire) >=
                    max_connections_)
                {
                    spdlog::warn("Too many connections, refusing client");
                    socket.close();
                    continue;
                }

                socket.set_option(ip::tcp::no_delay(true));
                socket.set_option(ip::tcp::socket::keep_alive(true));

                const auto session = std::make_shared<
                    worms_server::user_session>(std::move(socket));
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
        co_spawn(io_context_, listener(), detached);
        spdlog::info("Press Ctrl+C to exit");

        for (size_t i = 0; i < thread_count - 1; ++i)
        {
            threads_.emplace_back([this]()
            {
                io_context_.run();
            });
        }

        io_context_.run();
    }

    void server::stop()
    {
        io_context_.stop();
    }
}
