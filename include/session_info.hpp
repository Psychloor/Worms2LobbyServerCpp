//
// Created by blomq on 2025-07-10.
//

#ifndef SESSION_INFO_HPP
#define SESSION_INFO_HPP

#include <algorithm>
#include <array>
#include <cstdint>

#include "nation.hpp"

#include <boost/endian/conversion.hpp>

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
        nation nation = nation::team17;
        uint8_t game_version{};
        uint8_t game_release{};
        session_type type = session_type::user;
        session_access access = session_access::public_access;
        uint8_t always_one{};
        uint8_t always_zero{};
        std::array<net::byte, padding_size> padding{};

        session_info() = default;

        session_info(const worms_server::nation nation,
            const session_type type,
            const session_access access =
                session_access::public_access)
        {
            this->nation = nation;
            this->type = type;
            this->access = access;

            crc1 = 0x17171717;
            crc2 = 0x02010101;
            always_one = 1;
            always_zero = 0;
            game_release = 49;
            game_version = 49;

            padding.fill(static_cast<net::byte>(0));
        }

        static net::deserialization_result<session_info, std::string>
        read_from(net::packet_reader& reader);
    };

    inline void write_session_info(net::packet_writer& writer,
        const session_info& info)
    {
        writer.write_le(info.crc1);
        writer.write_le(info.crc2);
        writer.write_le(static_cast<uint8_t>(info.nation));
        writer.write_le(info.game_version);
        writer.write_le(info.game_release);
        writer.write_le(static_cast<uint8_t>(info.type));
        writer.write_le(static_cast<uint8_t>(info.access));
        writer.write_le(info.always_one);
        writer.write_le(info.always_zero);
        writer.write(info.padding);
    }

    [[nodiscard]] static bool verify_session_info(const session_info& info)
    {
        if (info.crc1 != 0x17171717U || info.crc2 !=
            boost::endian::little_to_native(0x02010101U))
        {
            spdlog::error("CRC Missmatch - CRC1: {} CRC2: {}",
                info.crc1,
                info.crc2);
            return false;
        }

        if (info.always_one != 1 || info.always_zero != 0)
        {
            spdlog::error(
                "Always one/zero mismatch - Always one: {} Always zero: {}",
                info.always_one,
                info.always_zero);
            return false;
        }

        const auto result = std::ranges::find_if_not(
            info.padding,
            [](const auto& value) { return value == net::byte{0}; });

        if (result != std::end(info.padding))
        {
            spdlog::error("Padding contains non-zero bytes");
            return false;
        }

        return true;
    }

    inline net::deserialization_result<session_info, std::string>
    session_info::read_from(net::packet_reader& reader)
    {
        session_info info;

        info.crc1 = reader.read_le<uint32_t>().value();
        info.crc2 = reader.read_le<uint32_t>().value();
        info.nation = static_cast<worms_server::nation>(reader.read_le<uint8_t>().value());
        info.game_version = reader.read_le<uint8_t>().value();
        info.game_release = reader.read_le<uint8_t>().value();
        info.type = static_cast<session_type>(reader.read_le<uint8_t>().value());
        info.access = static_cast<session_access>(reader.read_le<uint8_t>().value());
        info.always_one = reader.read_le<uint8_t>().value();
        info.always_zero = reader.read_le<uint8_t>().value();

        const auto padding_bytes = reader.read_bytes(padding_size).value();
        std::ranges::copy(padding_bytes, std::begin(info.padding));

        if (!verify_session_info(info))
        {
            return {
                .status = net::packet_parse_status::error,
                .error = "Invalid session info"
            };
        }

        return {
            .status = net::packet_parse_status::complete,
            .data = std::make_optional(info)
        };
    };
}

#endif //SESSION_INFO_HPP
