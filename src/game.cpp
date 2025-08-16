//
// Created by blomq on 2025-07-10.
//

#include "game.hpp"

#include <utility>

namespace worms_server
{
    Game::Game(const uint32_t id, const std::string_view name, const Nation nation, const uint32_t roomId,
               asio::ip::address_v4 address, const SessionAccess access) :
        id_(id), name_(name), sessionInfo_{nation, SessionType::Game, access}, address_(std::move(address)),
        roomId_(roomId)
    {
    }

    uint32_t Game::getId() const
    {
        return id_;
    }

    std::string_view Game::getName() const
    {
        return name_;
    }

    const SessionInfo& Game::getSessionInfo() const
    {
        return sessionInfo_;
    }

    asio::ip::address_v4 Game::getAddress() const
    {
        return address_;
    }

    uint32_t Game::getRoomId() const
    {
        return roomId_;
    }
} // namespace worms_server
