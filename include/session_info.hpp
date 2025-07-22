//
// Created by blomq on 2025-07-10.
//

#ifndef SESSION_INFO_HPP
#define SESSION_INFO_HPP

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>

#include "nation.hpp"

#include "framed_deserialization_result.hpp"
#include "packet_buffers.hpp"
#include "spdlog/spdlog.h"

namespace worms_server
{
    enum class session_type : uint8_t
    {
        room = 1,
        game = 4,
        user = 5
    };

    enum class session_access : uint8_t
    {
        public_access = 1,
        protected_access = 2,
    };

    struct session_info
    {
        static constexpr size_t padding_size = 35;
        uint32_t crc1{};
        uint32_t crc2{};
        nation player_nation = nation::team17;
        uint8_t game_version{};
        uint8_t game_release{};
        session_type type = session_type::user;
        session_access access = session_access::public_access;
        uint8_t always_one{};
        uint8_t always_zero{};
        std::array<net::byte, padding_size> padding{};

        session_info() = default;

        session_info(worms_server::nation nation, session_type type,
                     session_access access = session_access::public_access);

        void write_to(net::packet_writer& writer) const;

        [[nodiscard]] static net::deserialization_result<session_info,
                                                         std::string>
        read_from(net::packet_reader& reader);

        [[nodiscard]] static bool verify_session_info(const session_info& info);
    };
} // namespace worms_server

#endif // SESSION_INFO_HPP
