//
// Created by blomq on 2025-07-10.
//

#ifndef GAME_HPP
#define GAME_HPP

#include <asio/ip/address_v4.hpp>

#include "session_info.hpp"
#include "spdlog/spdlog.h"

namespace worms_server
{
    class game
    {
    public:
        explicit game(uint32_t id,
            std::string_view name,
            nation nation,
            uint32_t room_id,
            asio::ip::address_v4 address,
            session_access access);

        ~game()
        {
            spdlog::debug("Game {} has been destroyed", id_);
        }

        [[nodiscard]] uint32_t get_id() const;
        [[nodiscard]] std::string_view get_name() const;
        [[nodiscard]] const session_info& get_session_info() const;
        [[nodiscard]] asio::ip::address_v4 get_address() const;
        [[nodiscard]] uint32_t get_room_id() const;

    private:
        uint32_t id_;
        std::string name_;
        session_info session_info_;
        asio::ip::address_v4 address_;
        uint32_t room_id_;
    };
}

#endif //GAME_HPP
