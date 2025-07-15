//
// Created by blomq on 2025-07-10.
//

#include "user.hpp"

#include "user_session.hpp"

worms_server::user::user(const std::shared_ptr<user_session>& session,
    const uint32_t id,
    const std::string_view name,
    const nation nation) : id_(id), name_(name), session_info_(nation, session_type::user), session_(session)
{}

uint32_t worms_server::user::get_id() const
{
    return id_;
}

std::string_view worms_server::user::get_name() const
{
    return name_;
}

const worms_server::session_info& worms_server::user::get_session_info() const
{
    return session_info_;
}

uint32_t worms_server::user::get_room_id() const
{
    return room_id_.load(std::memory_order_acquire);
}

void worms_server::user::set_room_id(const uint32_t room_id)
{
    room_id_.store(room_id, std::memory_order_release);
}

void worms_server::user::send_packet(const net::shared_bytes_ptr& packet) const
{
    if (auto session = session_.lock())
    {
        session->send_packet(packet);
    }
}

boost::asio::ip::address_v4 worms_server::user::get_address() const
{
    if (!session_.expired())
    {
        if (const auto session = session_.lock(); session != nullptr)
        {
            return session->address_v4();
        }
    }

    return {};
}
