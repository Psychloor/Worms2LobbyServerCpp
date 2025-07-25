//
// Created by blomq on 2025-07-10.
//

#ifndef PACKET_FLAGS_HPP
#define PACKET_FLAGS_HPP

#include <cstdint>

namespace worms_server {
    enum class packet_flags : uint32_t {
        value0       = 1 << 0,
        value1       = 1 << 1,
        value2       = 1 << 2,
        value3       = 1 << 3,
        value4       = 1 << 4,
        value10      = 1 << 10,
        data_length  = 1 << 5,
        data         = 1 << 6,
        error        = 1 << 7,
        name         = 1 << 8,
        session_info = 1 << 9,
    };

    // Helper function to test flags
    static constexpr bool has_flag(const uint32_t value, const packet_flags flag) {
        return (value & static_cast<uint32_t>(flag)) != 0;
    }
} // namespace worms_server

#endif // PACKET_FLAGS_HPP
