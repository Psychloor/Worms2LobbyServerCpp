//
// Created by blomq on 2025-07-10.
//

#ifndef SESSION_INFO_HPP
#define SESSION_INFO_HPP

#include "spdlog/spdlog.h"
#include <array>
#include <cstdint>

#include "framed_packet_reader.hpp"
#include "nation.hpp"
#include "packet_buffers.hpp"

namespace worms_server {
    enum class SessionType : uint8_t { Room = 1, Game = 4, User = 5 };

    enum class SessionAccess : uint8_t {
        PublicAccess    = 1,
        ProtectedAccess = 2,
    };

    struct SessionInfo {
        static constexpr size_t PADDING_SIZE = 35;
        uint32_t crc1{};
        uint32_t crc2{};
        Nation playerNation = Nation::CustomTeam17Flag;
        uint8_t gameVersion{};
        uint8_t gameRelease{};
        SessionType type     = SessionType::User;
        SessionAccess access = SessionAccess::PublicAccess;
        uint8_t alwaysOne{};
        uint8_t alwaysZero{};
        std::array<net::byte, PADDING_SIZE> padding{};

        SessionInfo() = default;

        SessionInfo(
            worms_server::Nation nation, SessionType type, SessionAccess access = SessionAccess::PublicAccess);

        void writeTo(net::packet_writer& writer) const;

        [[nodiscard]] static net::deserialization_result<SessionInfo, std::string> readFrom(
            net::packet_reader& reader);

        [[nodiscard]] static bool verifySessionInfo(const SessionInfo& info);
    };
} // namespace worms_server

#endif // SESSION_INFO_HPP
