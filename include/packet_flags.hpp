//
// Created by blomq on 2025-07-10.
//

#ifndef PACKET_FLAGS_HPP
#define PACKET_FLAGS_HPP

#include <cstdint>

namespace worms_server {
    enum class PacketFlags : uint32_t {
        Value0       = 1 << 0,
        Value1       = 1 << 1,
        Value2       = 1 << 2,
        Value3       = 1 << 3,
        Value4       = 1 << 4,
        Value10      = 1 << 10,
        DataLength  = 1 << 5,
        Data         = 1 << 6,
        Error        = 1 << 7,
        Name         = 1 << 8,
        SessionInfo = 1 << 9,
    };

    // Helper function to test flags
    static constexpr bool HasFlag(const uint32_t value, const PacketFlags flag) {
        return (value & static_cast<uint32_t>(flag)) != 0;
    }
} // namespace worms_server

#endif // PACKET_FLAGS_HPP
