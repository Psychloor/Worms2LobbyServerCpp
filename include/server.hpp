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
    class server
    {
    public:
        server(uint16_t port, size_t max_connections);

        void run(size_t thread_count);
        void stop();

        static std::atomic_uint32_t connection_count;

    private:
        awaitable<void> listener();

        uint16_t port_;
        size_t max_connections_;

        thread_pool thread_pool_;
        io_context io_context_;
        signal_set signals_;
    };
}

#endif //SERVER_HPP
