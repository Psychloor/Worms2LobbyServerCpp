//
// Created by blomq on 2025-07-10.
//

#include "user.hpp"

#include "user_session.hpp"

worms_server::User::User(
    const std::shared_ptr<UserSession>& session, const uint32_t id, const std::string_view name, const Nation nation) :
    id_(id), name_(name), sessionInfo_(nation, SessionType::User), session_(session)
{
}

uint32_t worms_server::User::getId() const
{
    return id_;
}

std::string_view worms_server::User::getName() const
{
    return name_;
}

const worms_server::SessionInfo& worms_server::User::getSessionInfo() const
{
    return sessionInfo_;
}

uint32_t worms_server::User::getRoomId() const
{
    return roomId_.load(std::memory_order_relaxed);
}

void worms_server::User::setRoomId(const uint32_t roomId)
{
    roomId_.store(roomId, std::memory_order_release);
}

void worms_server::User::sendPacket(const net::shared_bytes_ptr& packet) const
{
    if (const auto session = session_.lock())
    {
        session->sendPacket(packet);
    }
}

asio::ip::address_v4 worms_server::User::getAddress() const
{
    if (const auto session = session_.lock())
    {
        return session->addressV4();
    }

    return {};
}
