//
// Created by blomq on 2025-07-10.
//

#ifndef USER_HPP
#define USER_HPP

#include "spdlog/spdlog.h"
#include <memory>
#include <string>

#include "session_info.hpp"
#include "worms_packet.hpp"
#include <asio/ip/address_v4.hpp>

namespace worms_server {
    class user_session;

    class user {
    public:
        explicit user(const std::shared_ptr<user_session>& session, uint32_t id, std::string_view name, nation nation);

        ~user() {
            spdlog::debug("User {} has been destroyed", id_);
        }

        [[nodiscard]] uint32_t get_id() const;
        [[nodiscard]] std::string_view get_name() const;
        [[nodiscard]] const session_info& get_session_info() const;
        [[nodiscard]] uint32_t get_room_id() const;
        void set_room_id(uint32_t room_id);

        void send_packet(const net::shared_bytes_ptr& packet) const;

        asio::ip::address_v4 get_address() const;

    private:
        uint32_t id_;
        std::string name_;
        session_info session_info_;
        std::atomic<uint32_t> room_id_;
        std::weak_ptr<user_session> session_;
    };
} // namespace worms_server

#endif // USER_HPP
