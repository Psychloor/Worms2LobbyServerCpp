//
// Created by blomq on 2025-06-24.
//

#ifndef PACKET_CODE_HPP
#define PACKET_CODE_HPP

#include <cstdint>

namespace worms_server
{
    enum class packet_code : uint32_t
    {
        list_rooms = 200,
        list_item = 350,
        list_end = 351,
        list_users = 400,
        list_games = 500,
        login = 600,
        login_reply = 601,
        create_room = 700,
        create_room_reply = 701,
        join = 800,
        join_reply = 801,
        leave = 900,
        leave_reply = 901,
        disconnect_user = 1000,
        close = 1100,
        close_reply = 1101,
        create_game = 1200,
        create_game_reply = 1201,
        chat_room = 1300,
        chat_room_reply = 1301,
        connect_game = 1326,
        connect_game_reply = 1327,
    };

    static constexpr bool packet_code_exists(const uint32_t code)
    {
        switch (code)
        {
        case static_cast<uint32_t>(packet_code::list_rooms):
        case static_cast<uint32_t>(packet_code::list_item):
        case static_cast<uint32_t>(packet_code::list_end):
        case static_cast<uint32_t>(packet_code::list_users):
        case static_cast<uint32_t>(packet_code::list_games):
        case static_cast<uint32_t>(packet_code::login):
        case static_cast<uint32_t>(packet_code::login_reply):
        case static_cast<uint32_t>(packet_code::create_room):
        case static_cast<uint32_t>(packet_code::create_room_reply):
        case static_cast<uint32_t>(packet_code::join):
        case static_cast<uint32_t>(packet_code::join_reply):
        case static_cast<uint32_t>(packet_code::leave):
        case static_cast<uint32_t>(packet_code::leave_reply):
        case static_cast<uint32_t>(packet_code::disconnect_user):
        case static_cast<uint32_t>(packet_code::close):
        case static_cast<uint32_t>(packet_code::close_reply):
        case static_cast<uint32_t>(packet_code::create_game):
        case static_cast<uint32_t>(packet_code::create_game_reply):
        case static_cast<uint32_t>(packet_code::chat_room):
        case static_cast<uint32_t>(packet_code::chat_room_reply):
        case static_cast<uint32_t>(packet_code::connect_game):
        case static_cast<uint32_t>(packet_code::connect_game_reply):
            return true;
        default:
            return false;
        }
    }
}

#endif //PACKET_CODE_HPP
