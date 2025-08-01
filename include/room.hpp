﻿//
// Created by blomq on 2025-07-10.
//

#ifndef ROOM_HPP
#define ROOM_HPP

#include <asio/ip/address_v4.hpp>
#include <shared_mutex>
#include <string>

#include "session_info.hpp"
#include "spdlog/spdlog.h"

namespace worms_server
{
    class user;
    class room
    {
    public:
        explicit room(uint32_t id, std::string_view name, nation nation,
                      asio::ip::address_v4 address);

        ~room() { spdlog::debug("Room {} has been destroyed", id_); }

        [[nodiscard]] uint32_t get_id() const;
        [[nodiscard]] std::string_view get_name() const;
        [[nodiscard]] const session_info& get_session_info() const;
        [[nodiscard]] asio::ip::address_v4 get_address() const;

    private:
        uint32_t id_;
        std::string name_;
        session_info session_info_;
        asio::ip::address_v4 address_;
    };
} // namespace worms_server

#endif // ROOM_HPP
