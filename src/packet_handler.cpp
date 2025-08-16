//
// Created by blomq on 2025-07-14.
//

#include "packet_handler.hpp"

#include "spdlog/spdlog.h"

#include <string_view>
#include "database.hpp"
#include "game.hpp"
#include "packet_code.hpp"
#include "room.hpp"
#include "string_utils.hpp"
#include "user.hpp"
#include "worms_packet.hpp"

using namespace std::string_view_literals;

namespace
{
    using namespace worms_server;

    awaitable<void> LeaveRoom(const std::shared_ptr<Room>& room, uint32_t leftId)
    {
        const auto database = Database::getInstance();
        const auto users = database->getUsers();
        const uint32_t roomId = room == nullptr ? 5 : room->getId();

        const bool roomClosed = room != nullptr && !std::ranges::any_of(users, [leftId, roomId](const auto& user)
        {
            return user->getId() != leftId && user->getRoomId() == roomId;
        }) && !std::ranges::any_of(database->getGames(), [leftId, roomId](const auto& game)
        {
            return game->getId() != leftId && game->getRoomId() == roomId;
        });

        if (roomClosed)
        {
            database->removeRoom(roomId);
        }

        const auto roomLeavePacketBytes =
            WormsPacket::freeze(PacketCode::Leave, {.value2 = roomId, .value10 = leftId});
        const auto roomClosePacketBytes = WormsPacket::freeze(PacketCode::Close, {.value10 = roomId});

        for (const auto& user : users)
        {
            if (user->getId() == leftId)
            {
                continue;
            }

            if (room != nullptr)
            {
                user->sendPacket(roomLeavePacketBytes);
            }

            if (roomClosed)
            {
                user->sendPacket(roomClosePacketBytes);
            }
        }

        co_return;
    }

    awaitable<bool> OnChatRoom(const std::shared_ptr<User>& clientUser,
                               const std::shared_ptr<Database>& database, const WormsPacketPtr& packet)
    {
        if (packet->fields().value0.value_or(0) != clientUser->getId() || !packet->fields().value3
            || !packet->fields().data)
        {
            spdlog::error("Invalid packet data\n");
            co_return false;
        }

        const auto& targetId = *packet->fields().value3;
        const auto clientRoomId = clientUser->getRoomId();
        const std::string_view message = *packet->fields().data;
        const auto clientId = clientUser->getId();
        const std::string_view clientName = clientUser->getName();

        if (message.starts_with(std::format("GRP:[ {} ]  "sv, clientName)))
        {
            // Check if the user can access the room.
            if (clientRoomId == targetId)
            {
                // Notify all users of the room.
                const auto packetBytes = WormsPacket::freeze(
                    PacketCode::ChatRoom, {.value0 = clientId, .value3 = clientRoomId, .data = message.data()});

                for (const auto& user : database->getUsers())
                {
                    if (user->getRoomId() == clientRoomId && user->getId() != clientId)
                    {
                        user->sendPacket(packetBytes);
                    }
                }

                // Notify sender
                clientUser->sendPacket(WormsPacket::freeze(PacketCode::ChatRoomReply, {.error = 0}));
                co_return true;
            }

            // Notify sender
            clientUser->sendPacket(WormsPacket::freeze(PacketCode::ChatRoomReply, {.error = 1}));
            co_return true;
        }

        if (message.starts_with(std::format("PRV:[ {} ]  "sv, clientName)))
        {
            const auto& targetUser = database->getUser(targetId);
            if (targetUser == nullptr || targetUser->getRoomId() != clientRoomId)
            {
                clientUser->sendPacket(WormsPacket::freeze(PacketCode::ChatRoomReply, {.error = 1}));
                co_return true;
            }

            // Notify Target
            targetUser->sendPacket(WormsPacket::freeze(
                PacketCode::ChatRoom, {.value0 = clientId, .value3 = targetId, .data = message.data()}));

            // Notify Sender
            clientUser->sendPacket(WormsPacket::freeze(PacketCode::ChatRoomReply, {.error = 0}));
            co_return true;
        }

        co_return true;
    }

    awaitable<bool> OnListRooms(const std::shared_ptr<User>& clientUser,
                                const std::shared_ptr<Database>& database, const WormsPacketPtr& packet)
    {
        if (packet->fields().value4.value_or(0) != 0)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        for (const auto rooms = database->getRooms(); const auto& room : rooms)
        {
            clientUser->sendPacket(
                WormsPacket::freeze(PacketCode::ListItem, {.value1 = room->getId(),
                                                           .name = std::string(room->getName()),
                                                           .data = "",
                                                           .info = room->getSessionInfo()}));
        }

        clientUser->sendPacket(WormsPacket::getListEndPacket());

        co_return true;
    }

    awaitable<bool> OnListUsers(const std::shared_ptr<User>& clientUser,
                                const std::shared_ptr<Database>& database, const WormsPacketPtr& packet)
    {
        if (packet->fields().value4.value_or(0) != 0
            || packet->fields().value2.value_or(0) != clientUser->getRoomId())
        {
            spdlog::error("List Users: Invalid packet data");
            co_return false;
        }

        const auto users = database->getUsers();
        const auto roomId = clientUser->getRoomId();
        for (const auto& user : users)
        {
            if (user->getRoomId() != roomId)
            {
                continue;
            }

            clientUser->sendPacket(
                WormsPacket::freeze(PacketCode::ListItem, {.value1 = user->getId(),
                                                           .name = std::string(user->getName()),
                                                           .data = "",
                                                           .info = user->getSessionInfo()}));
        }

        clientUser->sendPacket(WormsPacket::getListEndPacket());

        co_return true;
    }

    awaitable<bool> OnListGames(const std::shared_ptr<User>& clientUser,
                                const std::shared_ptr<Database>& database, const WormsPacketPtr& packet)
    {
        if (packet->fields().value4.value_or(0) != 0
            || packet->fields().value2.value_or(0) != clientUser->getRoomId())
        {
            spdlog::error("List Games: Invalid packet data");
            co_return false;
        }

        const auto games = database->getGames();
        for (const auto& game : games)
        {
            if (game->getRoomId() != clientUser->getRoomId())
            {
                continue;
            }

            clientUser->sendPacket(
                WormsPacket::freeze(PacketCode::ListItem, {.value1 = game->getId(),
                                                           .name = std::string(game->getName()),
                                                           .data = game->getAddress().to_string(),
                                                           .info = game->getSessionInfo()}));
        }

        clientUser->sendPacket(WormsPacket::getListEndPacket());

        co_return true;
    }

    awaitable<bool> OnCreateRoom(const std::shared_ptr<User>& clientUser,
                                 const std::shared_ptr<Database>& database, const WormsPacketPtr& packet)
    {
        if (packet->fields().value1.value_or(0) != 0 || packet->fields().value4.value_or(0) != 0
            || packet->fields().data.value_or("").empty() || packet->fields().name.value_or("").empty()
            || !packet->fields().info)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Check if the room name is valid is not already taken.
        const std::string_view requestedRoomName = *packet->fields().name;
        if (std::ranges::any_of(database->getRooms(), [requestedRoomName](const auto& room) -> bool
        {
            return EqualsCaseInsensitive(requestedRoomName, room->getName());
        }))
        {
            clientUser->sendPacket(WormsPacket::freeze(PacketCode::CreateRoomReply, {.value1 = 0, .error = 1}));

            co_return true;
        }

        const auto roomId = Database::getNextId();
        const auto room = std::make_shared<worms_server::Room>(
            roomId, *packet->fields().name, packet->fields().info->playerNation, clientUser->getAddress());
        Database::getInstance()->addRoom(room);


        const auto roomPacketBytes =
            WormsPacket::freeze(PacketCode::CreateRoom, {.value1 = roomId,
                                                         .value4 = 0,
                                                         .name = std::string(room->getName()),
                                                         .data = "",
                                                         .info = room->getSessionInfo()});

        // notify others
        for (const auto& user : database->getUsers())
        {
            if (user->getId() == clientUser->getId())
            {
                continue;
            }

            user->sendPacket(roomPacketBytes);
        }

        // Send the creation room reply packet
        clientUser->sendPacket(WormsPacket::freeze(PacketCode::CreateRoomReply, {.value1 = roomId, .error = 0}));

        co_return true;
    }

    awaitable<bool> OnJoin(const std::shared_ptr<User>& clientUser, const std::shared_ptr<Database>& database,
                           const WormsPacketPtr& packet)
    {
        if (!packet->fields().value2 || packet->fields().value10.value_or(0) != clientUser->getId())
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Require a valid room or game ID.
        // Check rooms
        if (std::ranges::any_of(database->getRooms(),
                                [join_id = packet->fields().value2](const auto& room) -> bool
                                {
                                    return room->getId() == join_id;
                                }))
        {
            clientUser->setRoomId(*packet->fields().value2);

            // Notify other users about the join.
            const auto packetBytes = WormsPacket::freeze(
                PacketCode::Join, {.value2 = packet->fields().value2, .value10 = clientUser->getId()});
            for (const auto& user : database->getUsers())
            {
                if (user->getId() == clientUser->getId())
                {
                    continue;
                }
                user->sendPacket(packetBytes);
            }

            clientUser->sendPacket(WormsPacket::freeze(PacketCode::JoinReply, {.error = 0}));
            co_return true;
        }

        // Check games
        if (std::ranges::any_of(database->getGames(),
                                [join_id = *packet->fields().value2, room_id = clientUser->getRoomId()](
                                const auto& game) -> bool
                                {
                                    return game->getId() == join_id && game->getRoomId() == room_id;
                                }))
        {
            // Notify other users about the join.
            const auto packetBytes = WormsPacket::freeze(
                PacketCode::Join, {.value2 = clientUser->getRoomId(), .value10 = clientUser->getId()});
            for (const auto& user : database->getUsers())
            {
                if (user->getId() == clientUser->getId())
                {
                    continue;
                }
                user->sendPacket(packetBytes);
            }

            clientUser->sendPacket(WormsPacket::freeze(PacketCode::JoinReply, {.error = 0}));
            co_return true;
        }

        // Reply to joiner. (failed to find)
        clientUser->sendPacket(WormsPacket::freeze(PacketCode::JoinReply, {.error = 1}));
        co_return true;
    }

    awaitable<bool> OnLeave(const std::shared_ptr<User>& clientUser, const std::shared_ptr<Database>& database,
                            const WormsPacketPtr& packet)
    {
        if (packet->fields().value10.value_or(0) != clientUser->getId() || !packet->fields().value2)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Require valid room ID (never sent for games, users disconnect if
        // leaving a game).
        if (packet->fields().value2 == clientUser->getRoomId())
        {
            co_await LeaveRoom(database->getRoom(clientUser->getRoomId()), clientUser->getId());
            clientUser->setRoomId(0);

            // Reply to leaver.
            clientUser->sendPacket(WormsPacket::freeze(PacketCode::LeaveReply, {.error = 0}));

            co_return true;
        }

        // Reply to leaver. (failed to find)
        clientUser->sendPacket(WormsPacket::freeze(PacketCode::LeaveReply, {.error = 1}));

        co_return true;
    }

    awaitable<bool> OnClose(const std::shared_ptr<User>& clientUser, const std::shared_ptr<Database>& database,
                            const WormsPacketPtr& packet)
    {
        if (!packet->fields().value10)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Never sent for games, users disconnect if leaving a game.
        // Reply success to the client, the server decides when to actually
        // close rooms.
        clientUser->sendPacket(WormsPacket::freeze(PacketCode::CloseReply, {.error = 0}));

        co_return true;
    }

    awaitable<bool> OnCreateGame(const std::shared_ptr<User>& clientUser,
                                 const std::shared_ptr<Database>& database, const WormsPacketPtr& packet)
    {
        if (packet->fields().value1.value_or(1) != 0
            || packet->fields().value2.value_or(0) != clientUser->getRoomId()
            || packet->fields().value4.value_or(0) != 0x800 || !packet->fields().data || !packet->fields().name
            || !packet->fields().info)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Require valid room ID and IP.
        ip::address parsedIp;
        try
        {
            parsedIp = ip::make_address(*packet->fields().data);
        }
        catch (const error_code& e)
        {
            spdlog::error("Invalid IP address: " + e.message());
            co_return false;
        }

        if (parsedIp.is_v4() && clientUser->getAddress() == parsedIp)
        {
            // Create a new game.
            const auto gameId = Database::getNextId();
            const auto game = std::make_shared<worms_server::Game>(gameId, clientUser->getName(),
                                                                   clientUser->getSessionInfo().playerNation,
                                                                   clientUser->getRoomId(), clientUser->getAddress(),
                                                                   packet->fields().info->access);
            database->addGame(game);

            // Notify other users about the new game, even those in other rooms.
            const auto packet_bytes =
                WormsPacket::freeze(PacketCode::CreateGame, {.value1 = gameId,
                                                             .value2 = game->getRoomId(),
                                                             .value4 = 0x800,
                                                             .name = std::string(game->getName()),
                                                             .data = game->getAddress().to_string(),
                                                             .info = game->getSessionInfo()});
            for (const auto& user : database->getUsers())
            {
                if (user->getId() == clientUser->getId())
                {
                    continue;
                }
                user->sendPacket(packet_bytes);
            }

            // Send reply to host;
            clientUser->sendPacket(
                WormsPacket::freeze(PacketCode::CreateGameReply, {.value1 = gameId, .error = 0}));
        }


        clientUser->sendPacket(WormsPacket::freeze(PacketCode::CreateGameReply, {.value1 = 0, .error = 2}));
        clientUser->sendPacket(WormsPacket::freeze(
            PacketCode::ChatRoom, {.value0 = clientUser->getId(),
                                   .value3 = clientUser->getRoomId(),
                                   .data = "GRP:Cannot host your game. Please use FrontendKitWS with "
                                   "fkNetcode. More information at worms2d.info/fkNetcode"}));

        co_return true;
    }

    awaitable<bool> OnConnectGame(const std::shared_ptr<User>& clientUser,
                                  const std::shared_ptr<Database>& database, const WormsPacketPtr& packet)
    {
        if (!packet->fields().value0)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Require valid game ID and user to be in appropriate room.
        const auto games = database->getGames();
        const auto gameId = packet->fields().value0;
        const auto roomId = clientUser->getRoomId();
        const auto it = std::ranges::find_if(games, [gameId, roomId](const auto& game) -> bool
        {
            return game->getId() == gameId && game->getRoomId() == roomId;
        });

        if (it == games.end())
        {
            clientUser->sendPacket(WormsPacket::freeze(PacketCode::ConnectGameReply, {.data = "", .error = 1}));
        }
        else
        {
            clientUser->sendPacket(WormsPacket::freeze(
                worms_server::PacketCode::ConnectGameReply, {.data = (*it)->getAddress().to_string(), .error = 0}));
        }

        co_return true;
    }
}

namespace worms_server
{


    awaitable<bool> PacketHandler::handlePacket(const std::shared_ptr<User>& clientUser,
                                                const std::shared_ptr<Database>& database, const WormsPacketPtr& packet)
    {
        switch (packet->code())
        {
        case PacketCode::ChatRoom:
            spdlog::debug("Chat room packet received");
            co_return co_await OnChatRoom(clientUser, database, packet);
            break;

        case PacketCode::ListRooms:
            spdlog::debug("List rooms packet received");
            co_return co_await OnListRooms(clientUser, database, packet);
            break;

        case PacketCode::ListUsers:
            spdlog::debug("List users packet received");
            co_return co_await OnListUsers(clientUser, database, packet);
            break;

        case PacketCode::ListGames:
            spdlog::debug("List games packet received");
            co_return co_await OnListGames(clientUser, database, packet);
            break;

        case PacketCode::CreateRoom:
            spdlog::debug("Create room packet received");
            co_return co_await OnCreateRoom(clientUser, database, packet);
            break;

        case PacketCode::Join:
            spdlog::debug("Join packet received");
            co_return co_await OnJoin(clientUser, database, packet);
            break;

        case PacketCode::Leave:
            spdlog::debug("Leave packet received");
            co_return co_await OnLeave(clientUser, database, packet);
            break;

        case PacketCode::Close:
            spdlog::debug("Close packet received");
            co_return co_await OnClose(clientUser, database, packet);
            break;

        case PacketCode::CreateGame:
            spdlog::debug("Create game packet received");
            co_return co_await OnCreateGame(clientUser, database, packet);
            break;

        case PacketCode::ConnectGame:
            spdlog::debug("Connect game packet received");
            co_return co_await OnConnectGame(clientUser, database, packet);
            break;

        default:
            spdlog::error("Unknown packet code: {}", static_cast<uint32_t>(packet->code()));
            break;
        }

        co_return false;
    }
} // namespace worms_server
