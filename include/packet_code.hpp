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
}

#endif //PACKET_CODE_HPP
