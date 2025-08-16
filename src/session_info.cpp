//
// Created by blomq on 2025-07-17.
//

#include "session_info.hpp"

namespace worms_server
{
    SessionInfo::SessionInfo(
        const worms_server::Nation nation, const SessionType type, const SessionAccess access) :
        crc1(0x17171717), crc2(0x02010101), playerNation(nation), gameVersion(49), gameRelease(49), type(type),
        access(access), alwaysOne(1)
    {
        padding.fill(static_cast<net::byte>(0));
    }

    void SessionInfo::writeTo(net::packet_writer& writer) const
    {
        writer.write_le(crc1);
        writer.write_le(crc2);
        writer.write_le(static_cast<uint8_t>(playerNation));
        writer.write_le(gameVersion);
        writer.write_le(gameRelease);
        writer.write_le(static_cast<uint8_t>(type));
        writer.write_le(static_cast<uint8_t>(access));
        writer.write_le(alwaysOne);
        writer.write_le(alwaysZero);
        writer.write(padding);
    }

    net::deserialization_result<SessionInfo, std::string> SessionInfo::readFrom(net::packet_reader& reader)
    {
        SessionInfo info;

        info.crc1 = *reader.read_le<uint32_t>();
        info.crc2 = *reader.read_le<uint32_t>();
        info.playerNation = static_cast<worms_server::Nation>(*reader.read_le<uint8_t>());
        info.gameVersion = *reader.read_le<uint8_t>();
        info.gameRelease = *reader.read_le<uint8_t>();
        info.type = static_cast<SessionType>(*reader.read_le<uint8_t>());
        info.access = static_cast<SessionAccess>(*reader.read_le<uint8_t>());
        info.alwaysOne = *reader.read_le<uint8_t>();
        info.alwaysZero = *reader.read_le<uint8_t>();

        const auto paddingBytes = *reader.read_bytes(PADDING_SIZE);
        std::ranges::copy(paddingBytes, std::begin(info.padding));

        if (!verifySessionInfo(info))
        {
            return {.status = net::packet_parse_status::error, .error = "Invalid session info"};
        }

        return {.status = net::packet_parse_status::complete, .data = std::make_optional(info)};
    }

    bool SessionInfo::verifySessionInfo(const SessionInfo& info)
    {
        static constexpr uint32_t EXPECTED_CRC2 =
            std::endian::native == std::endian::little ? 0x02010101U : std::byteswap(0x02010101U);

        if (info.crc1 != 0x17171717U || info.crc2 != EXPECTED_CRC2)
        {
            spdlog::error("CRC Missmatch - CRC1: {} CRC2: {}", info.crc1, info.crc2);
            return false;
        }

        if (info.alwaysOne != 1 || info.alwaysZero != 0)
        {
            spdlog::error(
                "Always one/zero mismatch - Always one: {} Always zero: {}", info.alwaysOne, info.alwaysZero);
            return false;
        }

        const auto result =
            std::ranges::find_if_not(info.padding, [](const auto& value) { return value == net::byte{0}; });

        if (result != std::end(info.padding))
        {
            spdlog::error("Padding contains non-zero bytes");
            return false;
        }

        return true;
    }

} // namespace worms_server
