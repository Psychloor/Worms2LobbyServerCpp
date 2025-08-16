//
// Created by blomq on 2025-06-24.
//

#ifndef PACKET_CODE_HPP
#define PACKET_CODE_HPP

#include <cstdint>

namespace worms_server
{
    enum class PacketCode : uint16_t
    {
        Unknown          = 0,
        ListRooms        = 200,
        ListItem         = 350,
        ListEnd          = 351,
        ListUsers        = 400,
        ListGames        = 500,
        Login            = 600,
        LoginReply       = 601,
        CreateRoom       = 700,
        CreateRoomReply  = 701,
        Join             = 800,
        JoinReply        = 801,
        Leave            = 900,
        LeaveReply       = 901,
        DisconnectUser   = 1000,
        Close            = 1100,
        CloseReply       = 1101,
        CreateGame       = 1200,
        CreateGameReply  = 1201,
        ChatRoom         = 1300,
        ChatRoomReply    = 1301,
        ConnectGame      = 1326,
        ConnectGameReply = 1327,
    };

    static constexpr bool PacketCodeExists(const uint32_t code)
    {
        switch (code)
        {
        case static_cast<uint32_t>(PacketCode::Login):
        case static_cast<uint32_t>(PacketCode::ListRooms):
        case static_cast<uint32_t>(PacketCode::ListUsers):
        case static_cast<uint32_t>(PacketCode::ListGames):
        case static_cast<uint32_t>(PacketCode::CreateRoom):
        case static_cast<uint32_t>(PacketCode::Join):
        case static_cast<uint32_t>(PacketCode::Leave):
        case static_cast<uint32_t>(PacketCode::Close):
        case static_cast<uint32_t>(PacketCode::CreateGame):
        case static_cast<uint32_t>(PacketCode::ChatRoom):
        case static_cast<uint32_t>(PacketCode::ConnectGame):
            return true;

        default:
            return false;
        }
    }
} // namespace worms_server

#endif // PACKET_CODE_HPP
