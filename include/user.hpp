//
// Created by blomq on 2025-07-10.
//

#ifndef USER_HPP
#define USER_HPP

#include <memory>
#include <string>
#include "spdlog/spdlog.h"

#include <asio/ip/address_v4.hpp>
#include "session_info.hpp"
#include "worms_packet.hpp"

namespace worms_server {
    class UserSession;

    class User {
    public:
        explicit User(const std::shared_ptr<UserSession>& session, uint32_t id, std::string_view name, Nation nation);

        ~User() {
            spdlog::debug("User {} has been destroyed", id_);
        }

        [[nodiscard]] uint32_t getId() const;
        [[nodiscard]] std::string_view getName() const;
        [[nodiscard]] const SessionInfo& getSessionInfo() const;
        [[nodiscard]] uint32_t getRoomId() const;
        void setRoomId(uint32_t roomId);

        void sendPacket(const net::shared_bytes_ptr& packet) const;

        asio::ip::address_v4 getAddress() const;

    private:
        uint32_t id_;
        std::string name_;
        SessionInfo sessionInfo_;
        std::atomic<uint32_t> roomId_;
        std::weak_ptr<UserSession> session_;
    };
} // namespace worms_server

#endif // USER_HPP
