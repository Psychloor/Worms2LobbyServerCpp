//
// Created by blomq on 2025-07-14.
//

#include "packet_handler.hpp"

#include "database.hpp"
#include "game.hpp"
#include "packet_code.hpp"
#include "room.hpp"
#include "string_utils.hpp"
#include "spdlog/spdlog.h"
#include "user.hpp"
#include "worms_packet.hpp"

namespace worms_server
{
    static awaitable<void> leave_room(const std::shared_ptr<room>& room,
        uint32_t left_id)
    {
        const auto database = database::get_instance();
        const auto users = database->get_users();
        const uint32_t room_id = room == nullptr ? 5 : room->get_id();

        const bool room_closed = room != nullptr
            && !std::ranges::any_of(users,
                [left_id, room_id](const auto& user)
                {
                    return user->get_id() != left_id && user->get_room_id() == room_id;
                })
            && !std::ranges::any_of(database->get_games(),
                [left_id, room_id](const auto& game)
                {
                    return game->get_id() != left_id && game->get_room_id() == room_id;
                });

        if (room_closed)
        {
            database->remove_room(room_id);
        }

        const auto room_leave_packet_bytes = worms_packet::freeze(
            packet_code::leave,
            {.value2 = room_id, .value10 = left_id});
        const auto room_close_packet_bytes = worms_packet::freeze(
            packet_code::close,
            {.value10 = room_id});
        for (const auto& user : users)
        {
            if (user->get_id() == left_id)
            {
                continue;
            }

            if (room != nullptr)
            {
                user->send_packet(room_leave_packet_bytes);
            }

            if (room_closed)
            {
                user->send_packet(room_close_packet_bytes);
            }
        }

        co_return;
    }

    static awaitable<bool> on_chat_room(
        const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        if (packet->fields().value0.value_or(0) != client_user->get_id() || !packet->fields().value3 || !packet->
            fields().data)
        {
            spdlog::error("Invalid packet data\n");
            co_return false;
        }

        const auto& target_id = packet->fields().value3.value();
        const auto client_room_id = client_user->get_room_id();
        const auto& message = packet->fields().data.value();
        const auto client_id = client_user->get_id();

        if (message.starts_with(std::format("GRP:[ {} ]  ", client_user->get_name())))
        {
            // Check if the user can access the room.
            if (client_room_id == target_id)
            {
                // Notify all users of the room.
                const auto packet_bytes = worms_packet::freeze(
                    packet_code::chat_room,
                    {
                        .value0 = client_id, .value3 = client_room_id,
                        .data = message
                    });

                for (const auto& user : database->get_users())
                {
                    if (user->get_room_id() == client_room_id && user->get_id() != client_id)
                    {
                        user->send_packet(packet_bytes);
                    }
                }

                // Notify sender
                client_user->send_packet(
                    worms_packet::freeze(packet_code::chat_room_reply,
                        {.error = 0}));
                co_return true;
            }

            // Notify sender
            client_user->send_packet(
                worms_packet::freeze(packet_code::chat_room_reply,
                    {.error = 1}));
            co_return true;
        }

        if (message.starts_with(
            std::format("PRV:[ {} ]  ", client_user->get_name())))
        {
            const auto& target_user = database->get_user(target_id);
            if (target_user == nullptr || target_user->get_room_id() != client_room_id)
            {
                client_user->send_packet(
                    worms_packet::freeze(packet_code::chat_room_reply,
                        {.error = 1}));
                co_return true;
            }

            // Notify Target
            target_user->send_packet(worms_packet::freeze(
                packet_code::chat_room,
                {.value0 = client_id, .value3 = target_id, .data = message}));

            // Notify Sender
            client_user->send_packet(
                worms_packet::freeze(packet_code::chat_room_reply,
                    {.error = 0}));
            co_return true;
        }

        co_return true;
    }

    static awaitable<bool> on_list_rooms(
        const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        if (packet->fields().value4.value_or(0) != 0)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        for (const auto rooms = database->get_rooms(); const auto& room : rooms)
        {
            client_user->send_packet(worms_packet::freeze(
                packet_code::list_item,
                {
                    .value1 = room->get_id(),
                    .name = std::string(room->get_name()), .data = "",
                    .session_info = room->get_session_info()
                }));
        }

        client_user->send_packet(worms_packet::get_list_end_packet());

        co_return true;
    }

    static awaitable<bool> on_list_users(
        const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        if (packet->fields().value4.value_or(0) != 0 || packet->fields().value2.
                                                                value_or(0) != client_user->get_room_id())
        {
            spdlog::error("List Users: Invalid packet data");
            co_return false;
        }

        const auto users = database->get_users();
        const auto room_id = client_user->get_room_id();
        for (const auto& user : users)
        {
            if (user->get_room_id() != room_id)
            {
                continue;
            }

            client_user->send_packet(worms_packet::freeze(
                packet_code::list_item,
                {
                    .value1 = user->get_id(),
                    .name = std::string(user->get_name()), .data = "",
                    .session_info = user->get_session_info()
                }));
        }

        client_user->send_packet(worms_packet::get_list_end_packet());

        co_return true;
    }

    static awaitable<bool> on_list_games(
        const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        if (packet->fields().value4.value_or(0) != 0 || packet->fields().value2.
                                                                value_or(0) != client_user->get_room_id())
        {
            spdlog::error("List Games: Invalid packet data");
            co_return false;
        }

        const auto games = database->get_games();
        for (const auto& game : games)
        {
            if (game->get_room_id() != client_user->get_room_id())
            {
                continue;
            }

            client_user->send_packet(worms_packet::freeze(
                packet_code::list_item,
                {
                    .value1 = game->get_id(),
                    .name = std::string(game->get_name()),
                    .data = game->get_address().to_string(),
                    .session_info = game->get_session_info()
                }));
        }

        client_user->send_packet(worms_packet::get_list_end_packet());

        co_return true;
    }

    static awaitable<bool> on_create_room(
        const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        if (packet->fields().value1.value_or(0) != 0 || packet->fields().value4.
                                                                value_or(0) != 0 || packet->fields().data.value_or("").
            empty() ||
            packet->fields().name.value_or("").empty() || !packet->fields().
                                                                   session_info.has_value())
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Check if the room name is valid is not already taken.
        const auto requested_room_name = packet->fields().name.value();
        if (std::ranges::any_of(database->get_rooms(),
            [requested_room_name](const auto& room) -> bool
            {
                return equals_case_insensitive(
                    requested_room_name,
                    room->get_name());
            }))
        {
            client_user->send_packet(worms_packet::freeze(
                packet_code::create_room_reply,
                {.value1 = 0, .error = 1}));

            co_return true;
        }

        const auto room_id = database::get_next_id();
        const auto room = std::make_shared<worms_server::room>(
            room_id,
            packet->fields().name.value(),
            packet->fields().session_info->nation,
            client_user->get_address());
        database::get_instance()->add_room(room);


        const auto room_packet_bytes = worms_packet::freeze(
            packet_code::create_room,
            {
                .value1 = room_id, .value4 = 0,
                .name = std::string(room->get_name()), .data = "",
                .session_info = room->get_session_info()
            });

        // notify others
        for (const auto& user : database->get_users())
        {
            if (user->get_id() == client_user->get_id()) { continue; }

            user->send_packet(room_packet_bytes);
        }

        // Send the creation room reply packet
        client_user->send_packet(worms_packet::freeze(
            packet_code::create_room_reply,
            {.value1 = room_id, .error = 0}));

        co_return true;
    }

    static awaitable<bool> on_join(const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        if (!packet->fields().value2 || packet->fields().value10.value_or(0) !=
            client_user->get_id())
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Require a valid room or game ID.
        // Check rooms
        if (std::ranges::any_of(database->get_rooms(),
            [join_id = packet->fields().value2.value()](
            const auto& room) -> bool
            {
                return room->get_id() == join_id;
            }))
        {
            client_user->set_room_id(packet->fields().value2.value());

            // Notify other users about the join.
            const auto packet_bytes = worms_packet::freeze(packet_code::join,
                {
                    .value2 = packet->fields().value2,
                    .value10 = client_user->get_id()
                });
            for (const auto& user : database->get_users())
            {
                if (user->get_id() == client_user->get_id()) { continue; }
                user->send_packet(packet_bytes);
            }

            client_user->send_packet(
                worms_packet::freeze(packet_code::join_reply, {.error = 0}));
            co_return true;
        }

        // Check games
        if (std::ranges::any_of(database->get_games(),
            [join_id = packet->fields().value2.value(),
                room_id = client_user->get_room_id()](
            const auto& game) -> bool
            {
                return game->get_id() == join_id && game->
                    get_room_id() == room_id;
            }))
        {
            // Notify other users about the join.
            const auto packet_bytes = worms_packet::freeze(packet_code::join,
                {
                    .value2 = client_user->get_room_id(),
                    .value10 = client_user->get_id()
                });
            for (const auto& user : database->get_users())
            {
                if (user->get_id() == client_user->get_id()) { continue; }
                user->send_packet(packet_bytes);
            }

            client_user->send_packet(
                worms_packet::freeze(packet_code::join_reply, {.error = 0}));
            co_return true;
        }

        // Reply to joiner. (failed to find)
        client_user->send_packet(
            worms_packet::freeze(packet_code::join_reply, {.error = 1}));
        co_return true;
    }

    static awaitable<bool> on_leave(const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        if (packet->fields().value10.value_or(0) != client_user->get_id() || !
            packet->fields().value2)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Require valid room ID (never sent for games, users disconnect if leaving a game).
        if (packet->fields().value2.value() == client_user->get_room_id())
        {
            co_await leave_room(database->get_room(client_user->get_room_id()),
                client_user->get_id());
            client_user->set_room_id(0);

            // Reply to leaver.
            client_user->send_packet(
                worms_packet::freeze(packet_code::leave_reply, {.error = 0}));

            co_return true;
        }

        // Reply to leaver. (failed to find)
        client_user->send_packet(
            worms_packet::freeze(packet_code::leave_reply, {.error = 1}));

        co_return true;
    }

    static awaitable<bool> on_close(const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        if (!packet->fields().value10)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Never sent for games, users disconnect if leaving a game.
        // Reply success to the client, the server decides when to actually close rooms.
        client_user->send_packet(
            worms_packet::freeze(packet_code::close_reply, {.error = 0}));

        co_return true;
    }

    static awaitable<bool> on_create_game(
        const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        if (packet->fields().value1.value_or(1) != 0 || packet->fields().value2.
                                                                value_or(0) != client_user->get_room_id() || packet->
            fields().value4
           .value_or(0) != 0x800 || !packet->fields().data || !packet->fields().
                                                                       name || !packet->fields().session_info)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Require valid room ID and IP.
        ip::address parsed_ip;
        try
        {
            parsed_ip = ip::make_address(packet->fields().data.value());
        }
        catch (const boost::system::error_code& e)
        {
            spdlog::error("Invalid IP address: " + e.what());
            co_return false;
        }

        if (parsed_ip.is_v4() && client_user->get_address() == parsed_ip)
        {
            // Create a new game.
            const auto game_id = database::get_next_id();
            const auto game = std::make_shared<worms_server::game>(
                game_id,
                client_user->get_name(),
                client_user->get_session_info().nation,
                client_user->get_room_id(),
                client_user->get_address(),
                packet->fields().session_info.value().access);
            database->add_game(game);

            // Notify other users about the new game, even those in other rooms.
            const auto packet_bytes = worms_packet::freeze(
                packet_code::create_game,
                {
                    .value1 = game_id, .value2 = game->get_room_id(),
                    .value4 = 0x800, .name = std::string(game->get_name()),
                    .data = game->get_address().to_string(),
                    .session_info = game->get_session_info()
                });
            for (const auto& user : database->get_users())
            {
                if (user->get_id() == client_user->get_id()) { continue; }
                user->send_packet(packet_bytes);
            }

            // Send reply to host;
            client_user->send_packet(worms_packet::freeze(
                packet_code::create_game_reply,
                {.value1 = game_id, .error = 0}));
        }


        client_user->send_packet(worms_packet::freeze(
            packet_code::create_game_reply,
            {.value1 = 0, .error = 2}));
        client_user->send_packet(worms_packet::freeze(packet_code::chat_room,
            {
                .value0 = client_user->get_id(),
                .value3 = client_user->get_room_id(),
                .data =
                "GRP:Cannot host your game. Please use FrontendKitWS with fkNetcode. More information at worms2d.info/fkNetcode"
            }));

        co_return true;
    }

    static awaitable<bool> on_connect_game(
        const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        if (!packet->fields().value0)
        {
            spdlog::error("Invalid packet data");
            co_return false;
        }

        // Require valid game ID and user to be in appropriate room.
        const auto games = database->get_games();
        const auto game_id = packet->fields().value0.value();
        const auto room_id = client_user->get_room_id();
        const auto it = std::ranges::find_if(games,
            [game_id, room_id](const auto& game) -> bool
            {
                return game->get_id() ==
                    game_id && game->
                    get_room_id() == room_id;
            });

        if (it == games.end())
        {
            client_user->send_packet(worms_packet::freeze(
                packet_code::connect_game_reply,
                {.data = "", .error = 1}));
        }
        else
        {
            client_user->send_packet(worms_packet::freeze(
                packet_code::connect_game_reply,
                {.data = (*it)->get_address().to_string(), .error = 0}));
        }

        co_return true;
    }

    awaitable<bool> packet_handler::handle_packet(
        const std::shared_ptr<user>& client_user,
        const std::shared_ptr<database>& database,
        const worms_packet_ptr& packet)
    {
        switch (packet->code())
        {
            case packet_code::chat_room:
                spdlog::debug("Chat room packet received");
                co_return co_await on_chat_room(client_user, database, packet);
                break;

            case packet_code::list_rooms:
                spdlog::debug("List rooms packet received");
                co_return co_await on_list_rooms(client_user, database, packet);
                break;

            case packet_code::list_users:
                spdlog::debug("List users packet received");
                co_return co_await on_list_users(client_user, database, packet);
                break;

            case packet_code::list_games:
                spdlog::debug("List games packet received");
                co_return co_await on_list_games(client_user, database, packet);
                break;

            case packet_code::create_room:
                spdlog::debug("Create room packet received");
                co_return co_await
                    on_create_room(client_user, database, packet);
                break;

            case packet_code::join:
                spdlog::debug("Join packet received");
                co_return co_await on_join(client_user, database, packet);
                break;

            case packet_code::leave:
                spdlog::debug("Leave packet received");
                co_return co_await on_leave(client_user, database, packet);
                break;

            case packet_code::close:
                spdlog::debug("Close packet received");
                co_return co_await on_close(client_user, database, packet);
                break;

            case packet_code::create_game:
                spdlog::debug("Create game packet received");
                co_return co_await
                    on_create_game(client_user, database, packet);
                break;

            case packet_code::connect_game:
                spdlog::debug("Connect game packet received");
                co_return co_await on_connect_game(
                    client_user,
                    database,
                    packet);
                break;

            default:
                spdlog::error("Unknown packet code: {}",
                    static_cast<uint32_t>(packet->code()));
                break;
        }

        co_return false;
    }
}
