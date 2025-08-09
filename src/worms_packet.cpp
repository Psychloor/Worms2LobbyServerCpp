//
// Created by blomq on 2025-06-24.
//

#include "worms_packet.hpp"

#include <algorithm>
#include <array>
#include <ostream>
#include <utility>

#include "packet_code.hpp"
#include "packet_flags.hpp"
#include "windows_1251.hpp"
#include "windows_1252.hpp"

namespace worms_server {
    std::atomic<bool> WormsPacket::useWindows1252Encoding{false};

    net::shared_bytes_ptr WormsPacket::freeze(const PacketCode code, PacketFields fields) {
        net::packet_writer writer;
        WormsPacket{code, std::move(fields)}.writeTo(writer);
        return std::move(writer).to_shared();
    }

    WormsPacket::WormsPacket(const PacketCode code, PacketFields fields)
        : code_(code), flags_(0), fields_(std::move(fields)) {}

    net::deserialization_result<WormsPacketPtr, std::string> WormsPacket::readFrom(net::packet_reader& reader) {
        if (!reader.can_read(sizeof(uint32_t) * 2)) {
            return {.status = net::packet_parse_status::partial};
        }
        const uint32_t codeValue = *reader.read_le<uint32_t>();
        if (!PacketCodeExists(codeValue)) {
            return {
                .status = net::packet_parse_status::error, .error = std::format("Unknown packet code: {}", codeValue)};
        }

        const auto code  = static_cast<PacketCode>(codeValue);
        const auto flags = *reader.read_le<uint32_t>();
        auto packet      = std::make_shared<WormsPacket>(code);

        if (HasFlag(flags, PacketFlags::Value0)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value0 = *reader.read_le<uint32_t>();
        }

        if (HasFlag(flags, PacketFlags::Value1)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value1 = *reader.read_le<uint32_t>();
        }

        if (HasFlag(flags, PacketFlags::Value2)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value2 = *reader.read_le<uint32_t>();
        }

        if (HasFlag(flags, PacketFlags::Value3)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value3 = *reader.read_le<uint32_t>();
        }

        if (HasFlag(flags, PacketFlags::Value4)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value4 = *reader.read_le<uint32_t>();
        }

        if (HasFlag(flags, PacketFlags::Value10)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value10 = *reader.read_le<uint32_t>();
        }

        if (HasFlag(flags, PacketFlags::DataLength)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }

            const auto data_length = *reader.read_le<uint32_t>();
            if (data_length > MAX_DATA_LENGTH) {
                return {.status = net::packet_parse_status::error,
                    .error      = std::format("Data length is too big: {}", data_length)};
            }

            packet->setDataLength(data_length);
        }

        if (HasFlag(flags, PacketFlags::Data)) {
            if (!reader.can_read(packet->dataLength())) {
                return {.status = net::packet_parse_status::partial};
            }

            // Add size validation
            if (packet->dataLength() == 0) {
                packet->fields_.data = "";
            } else {
                const auto bytes = *reader.read_bytes(packet->dataLength());
                // Ensure we have at least one byte for null terminator
                if (bytes.empty() || bytes.back() != std::byte{0}) {
                    return {
                        .status = net::packet_parse_status::error, .error = "Invalid data: missing null terminator"};
                }

                const auto encoded = std::string(reinterpret_cast<const char*>(bytes.data()),
                    bytes.size() - 1 // Subtract 1 to exclude null terminator
                );

                // Add size check before decoding
                if (encoded.length() > MAX_DATA_LENGTH) {
                    return {.status = net::packet_parse_status::error,
                        .error      = "String too long: encoded data exceeds "
                                      "maximum length"};
                }

                const std::string decoded = decodeString(encoded);

                // Add size check after decoding
                if (decoded.length() > MAX_DATA_LENGTH) {
                    return {.status = net::packet_parse_status::error,
                        .error      = "String too long: decoded data exceeds "
                                      "maximum length"};
                }

                packet->fields_.data = decoded;
            }
        }

        if (HasFlag(flags, PacketFlags::Error)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.error = *reader.read_le<uint32_t>();
        }

        if (HasFlag(flags, PacketFlags::Name)) {
            if (!reader.can_read(MAX_NAME_LENGTH)) {
                return {.status = net::packet_parse_status::partial};
            }
            const auto nameEncodedBytes = *reader.read_bytes(MAX_NAME_LENGTH);

            // Find the first null terminator or use the whole buffer
            const auto terminatorPos = std::ranges::find(nameEncodedBytes, static_cast<std::byte>(0));
            const auto length         = static_cast<size_t>(terminatorPos - nameEncodedBytes.begin());

            const std::string nameEncoded(reinterpret_cast<const char*>(nameEncodedBytes.data()), length);

            if (nameEncoded.length() > MAX_NAME_LENGTH) {
                return {.status = net::packet_parse_status::error,
                    .error      = "Name too long: encoded name exceeds maximum length"};
            }

            const auto nameDecoded = decodeString(nameEncoded);

            if (nameDecoded.length() > MAX_NAME_LENGTH) {
                return {.status = net::packet_parse_status::error,
                    .error      = "Name too long: decoded name exceeds maximum length"};
            }

            packet->fields_.name = nameDecoded;
        }

        if (HasFlag(flags, PacketFlags::SessionInfo)) {
            if (!reader.can_read(50)) {
                return {.status = net::packet_parse_status::partial};
            }

            const auto [status, data, error] = SessionInfo::readFrom(reader);
            if (status == net::packet_parse_status::error) {
                return {.status = net::packet_parse_status::error, .error = error};
            }

            SessionInfo info    = *data;
            info.gameRelease    = 49;
            packet->fields_.info = info;
        }

        return {.status = net::packet_parse_status::complete, .data = std::make_optional(std::move(packet))};
    }

    std::string WormsPacket::encodeString(const std::string& input) {
        return useWindows1252Encoding.load(std::memory_order::relaxed) ? Windows1252::encode(input)
                                                                         : Windows1251::encode(input);
    }


    std::string WormsPacket::decodeString(const std::string& input) {
        return useWindows1252Encoding.load(std::memory_order::relaxed) ? Windows1252::decode(input)
                                                                         : Windows1251::decode(input);
    }

    void WormsPacket::writeTo(net::packet_writer& writer) const {
        writer.write_le(static_cast<uint32_t>(code_));
        writer.write_le(getFlagsFromFields());

        if (fields_.value0) {
            writer.write_le(*fields_.value0);
        }

        if (fields_.value1) {
            writer.write_le(*fields_.value1);
        }

        if (fields_.value2) {
            writer.write_le(*fields_.value2);
        }

        if (fields_.value3) {
            writer.write_le(*fields_.value3);
        }

        if (fields_.value4) {
            writer.write_le(*fields_.value4);
        }

        if (fields_.value10) {
            writer.write_le(*fields_.value10);
        }

        if (fields_.data) {
            const auto& str    = *fields_.data;
            const auto encoded = encodeString(str);

            writer.write_le(static_cast<uint32_t>(encoded.size() + 1));
            writer.write_bytes(std::as_bytes(std::span{encoded}));
            writer.write(std::byte{0});
        }

        if (fields_.error) {
            writer.write_le(*fields_.error);
        }

        if (fields_.name) {
            const auto encoded = encodeString(*fields_.name);

            const auto encodedBytes = std::as_bytes(std::span{encoded});

            // Name is a fixed size string of 20 chars
            std::array<net::byte, MAX_NAME_LENGTH> buffer{};
            std::ranges::fill(buffer, std::byte{0});

            const auto length = std::min(encodedBytes.size_bytes(), MAX_NAME_LENGTH);

            std::copy_n(std::begin(encodedBytes), length, std::begin(buffer));

            writer.write_bytes(buffer);
        }

        if (fields_.info) {
            const auto& info = fields_.info;
            info->writeTo(writer);
        }
    }

    PacketCode WormsPacket::code() const {
        return code_;
    }

    size_t WormsPacket::dataLength() const {
        return fields_.dataLength.value_or(0);
    }

    void WormsPacket::setDataLength(const size_t length) {
        flags_ |= static_cast<uint32_t>(PacketFlags::DataLength);
        fields_.dataLength = static_cast<uint32_t>(length);
    }

    const PacketFields& WormsPacket::fields() const {
        return fields_;
    }

    constexpr uint32_t WormsPacket::getFlagsFromFields() const {
        uint32_t flags = 0;
        if (fields_.value0) {
            flags |= static_cast<uint32_t>(PacketFlags::Value0);
        }
        if (fields_.value1) {
            flags |= static_cast<uint32_t>(PacketFlags::Value1);
        }
        if (fields_.value2) {
            flags |= static_cast<uint32_t>(PacketFlags::Value2);
        }
        if (fields_.value3) {
            flags |= static_cast<uint32_t>(PacketFlags::Value3);
        }
        if (fields_.value4) {
            flags |= static_cast<uint32_t>(PacketFlags::Value4);
        }
        if (fields_.value10) {
            flags |= static_cast<uint32_t>(PacketFlags::Value10);
        }
        if (fields_.dataLength || fields_.data) {
            flags |= static_cast<uint32_t>(PacketFlags::DataLength);
        }
        if (fields_.data) {
            flags |= static_cast<uint32_t>(PacketFlags::Data);
        }
        if (fields_.error) {
            flags |= static_cast<uint32_t>(PacketFlags::Error);
        }
        if (fields_.name) {
            flags |= static_cast<uint32_t>(PacketFlags::Name);
        }
        if (fields_.info) {
            flags |= static_cast<uint32_t>(PacketFlags::SessionInfo);
        }
        return flags;
    }
} // namespace worms_server
