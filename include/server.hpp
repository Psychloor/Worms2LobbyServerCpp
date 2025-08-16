//
// Created by blomq on 2025-07-14.
//

#ifndef SERVER_HPP
#define SERVER_HPP

#include <asio.hpp>

using asio::awaitable;
using asio::use_awaitable;
using namespace asio;

namespace worms_server
{
    class Server
    {
    public:
        Server(uint16_t port, size_t maxConnections);

        void run(size_t threadCount);
        void stop();

        static std::atomic_uint32_t connectionCount;

    private:
        awaitable<void> listener();

        uint16_t port_;
        size_t maxConnections_;

        thread_pool threadPool_;
        io_context ioContext_;
        signal_set signals_;
        bool running_;
    };
} // namespace worms_server

#endif // SERVER_HPP
