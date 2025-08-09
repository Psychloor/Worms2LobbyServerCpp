//
// Created by blomq on 2025-07-10.
//

#ifndef GAME_HPP
#define GAME_HPP

#include "spdlog/spdlog.h"

#include "session_info.hpp"
#include <asio/ip/address_v4.hpp>

namespace worms_server {
    class Game {
    public:
        explicit Game(uint32_t id, std::string_view name, Nation nation, uint32_t roomId, asio::ip::address_v4 address,
            SessionAccess access);

        ~Game() {
            spdlog::debug("Game {} has been destroyed", id_);
        }

        [[nodiscard]] uint32_t getId() const;
        [[nodiscard]] std::string_view getName() const;
        [[nodiscard]] const SessionInfo& getSessionInfo() const;
        [[nodiscard]] asio::ip::address_v4 getAddress() const;
        [[nodiscard]] uint32_t getRoomId() const;

    private:
        uint32_t id_;
        std::string name_;
        SessionInfo sessionInfo_;
        asio::ip::address_v4 address_;
        uint32_t roomId_;
    };
} // namespace worms_server

#endif // GAME_HPP
