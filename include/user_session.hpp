//
// Created by blomq on 2025-07-14.
//

#ifndef USER_SESSION_HPP
#define USER_SESSION_HPP

#include "concurrentqueue/concurrentqueue.h"

#include <asio.hpp>
#include "packet_buffers.hpp"

namespace worms_server {
    class Database;
    class Room;
    class User;
} // namespace worms_server

namespace worms_server {
    using asio::awaitable;
    using asio::use_awaitable;

    class UserSession final : public std::enable_shared_from_this<UserSession> {
    public:
        explicit UserSession(asio::ip::tcp::socket socket);
        ~UserSession();

        awaitable<void> run();

        void sendPacket(const net::shared_bytes_ptr& packet);
        asio::ip::address_v4 addressV4() const;

    private:
        awaitable<std::shared_ptr<User>> handleLogin();
        awaitable<void> handleSession();
        awaitable<void> writer();

        std::shared_ptr<Database> database_;
        std::atomic<bool> isShuttingDown_{false};
        asio::ip::tcp::socket socket_;

        std::shared_ptr<User> user_;

        asio::steady_timer timer_;
        moodycamel::ConcurrentQueue<net::shared_bytes_ptr> packets_;
        asio::strand<asio::any_io_executor> strand_;
    };
} // namespace worms_server

#endif // USER_SESSION_HPP
