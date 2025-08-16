//
// Created by blomq on 2025-07-14.
//

#include "server.hpp"

#include "spdlog/spdlog.h"

#include "user_session.hpp"


namespace worms_server
{
    std::atomic<unsigned int> Server::connectionCount{0};

    Server::Server(const uint16_t port, const size_t maxConnections) : // NOLINT(*-easily-swappable-parameters)
        port_(port), maxConnections_(maxConnections),
        threadPool_(std::max(1U, std::thread::hardware_concurrency())), signals_(ioContext_, SIGINT, SIGTERM)
    {
        signals_.async_wait([this](const error_code&, int) { stop(); });
    }

    awaitable<void> Server::listener()
    {
        auto executor = co_await this_coro::executor;
        ip::tcp::acceptor acceptor(executor, {ip::tcp::v4(), port_});

        if (acceptor.is_open())
        {
            acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
            spdlog::info("Listening on port {}", port_);
        }
        else
        {
            spdlog::error("Failed to open listener on port: {}", port_);
            co_return;
        }

        for (;;)
        {
            ip::tcp::socket socket(executor);
            error_code ec;

            co_await acceptor.async_accept(socket, redirect_error(use_awaitable, ec));

            if (!ec)
            {
                if (connectionCount.load(std::memory_order_acquire) >= maxConnections_)
                {
                    spdlog::warn("Too many connections, refusing client");
                    socket.close();
                    continue;
                }

                socket.set_option(ip::tcp::no_delay(true));
                socket.set_option(ip::tcp::socket::keep_alive(true));

                const auto session = std::make_shared<UserSession>(std::move(socket));
                co_spawn(ioContext_, std::move(session)->run(), detached);
            }
            else
            {
                spdlog::error("Accept error: {}", ec.message());
            }
        }
    }

    void Server::run(const size_t threadCount)
    {
        co_spawn(ioContext_, listener(), detached);
        spdlog::info("Press Ctrl+C to exit");

        for (size_t i = 0; i < threadCount - 1; ++i)
        {
            post(ioContext_, [this]() { ioContext_.run(); });
        }

        // Run the io_context on the main thread as well
        ioContext_.run();

        // Wait for the thread pool to complete
        threadPool_.join();
    }

    void Server::stop()
    {
        ioContext_.stop();
        threadPool_.stop();
    }
} // namespace worms_server
