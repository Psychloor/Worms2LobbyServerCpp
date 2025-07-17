//
// Created by blomq on 2025-07-14.
//

#ifndef USER_SESSION_HPP
#define USER_SESSION_HPP

#include <asio.hpp>
#include "packet_buffers.hpp"
#include "concurrentqueue/concurrentqueue.h"

namespace worms_server
{
    class database;
    class room;
    class user;
}

namespace worms_server
{
    using asio::awaitable;
    using asio::use_awaitable;

    class user_session final : public std::enable_shared_from_this<user_session>
    {
    public:
        explicit user_session(asio::ip::tcp::socket socket);
        ~user_session();

        awaitable<void> run();

        void send_packet(const net::shared_bytes_ptr& packet);
        asio::ip::address_v4 address_v4() const;

    private:
        awaitable<std::shared_ptr<user>> handle_login();
        awaitable<void> handle_session();
        awaitable<void> writer();

        std::shared_ptr<database> database_;
        std::atomic<bool> is_shutting_down_{false};
        asio::ip::tcp::socket socket_;

        std::shared_ptr<user> user_;

        asio::steady_timer timer_;
        moodycamel::ConcurrentQueue<net::shared_bytes_ptr> packets_;
    };
}

#endif //USER_SESSION_HPP
