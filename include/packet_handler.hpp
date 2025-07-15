//
// Created by blomq on 2025-07-14.
//

#ifndef PACKET_ROUTER_HPP
#define PACKET_ROUTER_HPP

#include <memory>
#include <boost/asio.hpp>

#include "worms_packet.hpp"

using boost::asio::awaitable;
using boost::asio::use_awaitable;
using namespace boost::asio;

namespace worms_server
{
    class user;
    class database;
    class worms_packet;
    enum class packet_code : std::uint32_t;

    class packet_handler final
    {
    public:
        static awaitable<bool> handle_packet(
            const std::shared_ptr<user>& client_user,
            const std::shared_ptr<database>& database,
            const worms_packet_ptr& packet);
    };
}

#endif //PACKET_ROUTER_HPP
