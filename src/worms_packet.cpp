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
    std::atomic<bool> worms_packet::use_windows1252_encoding{false};

    net::shared_bytes_ptr worms_packet::freeze(const packet_code code, packet_fields fields) {
        net::packet_writer writer;
        worms_packet{code, std::move(fields)}.write_to(writer);
        return std::move(writer).to_shared();
    }

    worms_packet::worms_packet(const packet_code code, packet_fields fields)
        : code_(code), flags_(0), fields_(std::move(fields)) {}

    net::deserialization_result<worms_packet_ptr, std::string> worms_packet::read_from(net::packet_reader& reader) {
        if (!reader.can_read(sizeof(uint32_t) * 2)) {
            return {.status = net::packet_parse_status::partial};
        }
        const uint32_t code_value = *reader.read_le<uint32_t>();
        if (!packet_code_exists(code_value)) {
            return {
                .status = net::packet_parse_status::error, .error = std::format("Unknown packet code: {}", code_value)};
        }

        const auto code  = static_cast<packet_code>(code_value);
        const auto flags = *reader.read_le<uint32_t>();
        auto packet      = std::make_shared<worms_packet>(code);

        if (has_flag(flags, packet_flags::value0)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value0 = *reader.read_le<uint32_t>();
        }

        if (has_flag(flags, packet_flags::value1)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value1 = *reader.read_le<uint32_t>();
        }

        if (has_flag(flags, packet_flags::value2)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value2 = *reader.read_le<uint32_t>();
        }

        if (has_flag(flags, packet_flags::value3)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value3 = *reader.read_le<uint32_t>();
        }

        if (has_flag(flags, packet_flags::value4)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value4 = *reader.read_le<uint32_t>();
        }

        if (has_flag(flags, packet_flags::value10)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.value10 = *reader.read_le<uint32_t>();
        }

        if (has_flag(flags, packet_flags::data_length)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }

            const auto data_length = *reader.read_le<uint32_t>();
            if (data_length > max_data_length) {
                return {.status = net::packet_parse_status::error,
                    .error      = std::format("Data length is too big: {}", data_length)};
            }

            packet->set_data_length(data_length);
        }

        if (has_flag(flags, packet_flags::data)) {
            if (!reader.can_read(packet->data_length())) {
                return {.status = net::packet_parse_status::partial};
            }

            // Add size validation
            if (packet->data_length() == 0) {
                packet->fields_.data = "";
            } else {
                const auto bytes = *reader.read_bytes(packet->data_length());
                // Ensure we have at least one byte for null terminator
                if (bytes.empty() || bytes.back() != std::byte{0}) {
                    return {
                        .status = net::packet_parse_status::error, .error = "Invalid data: missing null terminator"};
                }

                const auto encoded = std::string(reinterpret_cast<const char*>(bytes.data()),
                    bytes.size() - 1 // Subtract 1 to exclude null terminator
                );

                // Add size check before decoding
                if (encoded.length() > max_data_length) {
                    return {.status = net::packet_parse_status::error,
                        .error      = "String too long: encoded data exceeds "
                                      "maximum length"};
                }

                const std::string decoded = decode_string(encoded);

                // Add size check after decoding
                if (decoded.length() > max_data_length) {
                    return {.status = net::packet_parse_status::error,
                        .error      = "String too long: decoded data exceeds "
                                      "maximum length"};
                }

                packet->fields_.data = decoded;
            }
        }

        if (has_flag(flags, packet_flags::error)) {
            if (!reader.can_read(sizeof(uint32_t))) {
                return {.status = net::packet_parse_status::partial};
            }
            packet->fields_.error = *reader.read_le<uint32_t>();
        }

        if (has_flag(flags, packet_flags::name)) {
            if (!reader.can_read(max_name_length)) {
                return {.status = net::packet_parse_status::partial};
            }
            const auto name_encoded_bytes = *reader.read_bytes(max_name_length);

            // Find the first null terminator or use the whole buffer
            const auto terminator_pos = std::ranges::find(name_encoded_bytes, static_cast<std::byte>(0));
            const auto length         = static_cast<size_t>(terminator_pos - name_encoded_bytes.begin());

            const std::string name_encoded(reinterpret_cast<const char*>(name_encoded_bytes.data()), length);

            if (name_encoded.length() > max_name_length) {
                return {.status = net::packet_parse_status::error,
                    .error      = "Name too long: encoded name exceeds maximum length"};
            }

            const auto name_decoded = decode_string(name_encoded);

            if (name_decoded.length() > max_name_length) {
                return {.status = net::packet_parse_status::error,
                    .error      = "Name too long: decoded name exceeds maximum length"};
            }

            packet->fields_.name = name_decoded;
        }

        if (has_flag(flags, packet_flags::session_info)) {
            if (!reader.can_read(50)) {
                return {.status = net::packet_parse_status::partial};
            }

            const auto [status, data, error] = session_info::read_from(reader);
            if (status == net::packet_parse_status::error) {
                return {.status = net::packet_parse_status::error, .error = error};
            }

            session_info info    = *data;
            info.game_release    = 49;
            packet->fields_.info = info;
        }

        return {.status = net::packet_parse_status::complete, .data = std::make_optional(std::move(packet))};
    }

    std::string worms_packet::encode_string(const std::string& input) {
        return use_windows1252_encoding.load(std::memory_order::relaxed) ? windows_1252::encode(input)
                                                                         : windows_1251::encode(input);
    }


    std::string worms_packet::decode_string(const std::string& input) {
        return use_windows1252_encoding.load(std::memory_order::relaxed) ? windows_1252::decode(input)
                                                                         : windows_1251::decode(input);
    }

    void worms_packet::write_to(net::packet_writer& writer) const {
        writer.write_le(static_cast<uint32_t>(code_));
        writer.write_le(get_flags_from_fields());

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
            const auto encoded = encode_string(str);

            writer.write_le(static_cast<uint32_t>(encoded.size() + 1));
            writer.write_bytes(std::as_bytes(std::span{encoded}));
            writer.write(std::byte{0});
        }

        if (fields_.error) {
            writer.write_le(*fields_.error);
        }

        if (fields_.name) {
            const auto encoded = encode_string(*fields_.name);

            const auto encoded_bytes = std::as_bytes(std::span{encoded});

            // Name is a fixed size string of 20 chars
            std::array<net::byte, max_name_length> buffer{};
            std::ranges::fill(buffer, std::byte{0});

            const auto length = std::min(encoded_bytes.size_bytes(), max_name_length);

            std::copy_n(std::begin(encoded_bytes), length, std::begin(buffer));

            writer.write_bytes(buffer);
        }

        if (fields_.info) {
            const auto& info = fields_.info;
            info->write_to(writer);
        }
    }

    packet_code worms_packet::code() const {
        return code_;
    }

    size_t worms_packet::data_length() const {
        return fields_.data_length.value_or(0);
    }

    void worms_packet::set_data_length(const size_t length) {
        flags_ |= static_cast<uint32_t>(packet_flags::data_length);
        fields_.data_length = static_cast<uint32_t>(length);
    }

    const packet_fields& worms_packet::fields() const {
        return fields_;
    }

    constexpr uint32_t worms_packet::get_flags_from_fields() const {
        uint32_t flags = 0;
        if (fields_.value0) {
            flags |= static_cast<uint32_t>(packet_flags::value0);
        }
        if (fields_.value1) {
            flags |= static_cast<uint32_t>(packet_flags::value1);
        }
        if (fields_.value2) {
            flags |= static_cast<uint32_t>(packet_flags::value2);
        }
        if (fields_.value3) {
            flags |= static_cast<uint32_t>(packet_flags::value3);
        }
        if (fields_.value4) {
            flags |= static_cast<uint32_t>(packet_flags::value4);
        }
        if (fields_.value10) {
            flags |= static_cast<uint32_t>(packet_flags::value10);
        }
        if (fields_.data_length || fields_.data) {
            flags |= static_cast<uint32_t>(packet_flags::data_length);
        }
        if (fields_.data) {
            flags |= static_cast<uint32_t>(packet_flags::data);
        }
        if (fields_.error) {
            flags |= static_cast<uint32_t>(packet_flags::error);
        }
        if (fields_.name) {
            flags |= static_cast<uint32_t>(packet_flags::name);
        }
        if (fields_.info) {
            flags |= static_cast<uint32_t>(packet_flags::session_info);
        }
        return flags;
    }
} // namespace worms_server
