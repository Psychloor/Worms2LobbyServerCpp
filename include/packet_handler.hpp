//
// Created by blomq on 2025-07-14.
//

#ifndef PACKET_ROUTER_HPP
#define PACKET_ROUTER_HPP

#include <asio.hpp>
#include <memory>

#include "worms_packet.hpp"

using asio::awaitable;
using asio::use_awaitable;
using namespace asio;

namespace worms_server
{
    class User;
    class Database;
    class WormsPacket;
    enum class PacketCode : std::uint32_t;

    class PacketHandler final
    {
    public:
        static awaitable<bool> handlePacket(
            const std::shared_ptr<User>& clientUser,
            const std::shared_ptr<Database>& database,
            const WormsPacketPtr& packet);
    };
}

#endif //PACKET_ROUTER_HPP
