//
// Created by blomq on 2025-07-10.
//

#include "game.hpp"

#include <utility>

namespace worms_server
{
    game::game(const uint32_t id,
        const std::string_view name,
        const nation nation,
        const uint32_t room_id,
        boost::asio::ip::address_v4 address,
        const session_access access) : id_(id), name_(name),
                                       session_info_{ nation, session_type::game, access }, address_(std::move(address)),
                                       room_id_(room_id)
    {}

    uint32_t game::get_id() const
    {
        return id_;
    }

    std::string_view game::get_name() const
    {
        return name_;
    }

    const session_info& game::get_session_info() const
    {
        return session_info_;
    }

    boost::asio::ip::address_v4 game::get_address() const
    {
        return address_;
    }

    uint32_t game::get_room_id() const
    {
        return room_id_;
    }
}
