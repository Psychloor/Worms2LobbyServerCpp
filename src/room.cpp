//
// Created by blomq on 2025-07-10.
//

#include "room.hpp"

#include <utility>

#include "user.hpp"

namespace worms_server
{
    Room::Room(const uint32_t id, const std::string_view name,
               const Nation nation, asio::ip::address_v4 address) :
        id_(id), name_(name), sessionInfo_{nation, SessionType::Room},
        address_(std::move(address))
    {
    }

    uint32_t Room::getId() const { return id_; }

    std::string_view Room::getName() const { return name_; }

    const SessionInfo& Room::getSessionInfo() const { return sessionInfo_; }

    asio::ip::address_v4 Room::getAddress() const { return address_; }
} // namespace worms_server
