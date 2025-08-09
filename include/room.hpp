//
// Created by blomq on 2025-07-10.
//

#ifndef ROOM_HPP
#define ROOM_HPP

#include <string>
#include <asio/ip/address_v4.hpp>

#include "session_info.hpp"
#include "spdlog/spdlog.h"

namespace worms_server
{
    class User;
    class Room
    {
    public:
        explicit Room(uint32_t id, std::string_view name, Nation nation,
                      asio::ip::address_v4 address);

        ~Room() { spdlog::debug("Room {} has been destroyed", id_); }

        [[nodiscard]] uint32_t getId() const;
        [[nodiscard]] std::string_view getName() const;
        [[nodiscard]] const SessionInfo& getSessionInfo() const;
        [[nodiscard]] asio::ip::address_v4 getAddress() const;

    private:
        uint32_t id_;
        std::string name_;
        SessionInfo sessionInfo_;
        asio::ip::address_v4 address_;
    };
} // namespace worms_server

#endif // ROOM_HPP
