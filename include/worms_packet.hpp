//
// Created by blomq on 2025-06-24.
//

#ifndef WORMS_PACKET_HPP
#define WORMS_PACKET_HPP

#include <cstdint>
#include <optional>
#include <string>

#include "session_info.hpp"

namespace worms_server
{
    enum class packet_code : uint32_t;
    enum class packet_flags : uint32_t;
}

namespace worms_server
{
    class worms_packet
    {
    public:
        explicit worms_packet(packet_code code);

    private:
        packet_code _code;
        packet_flags _flags;
        std::optional<uint32_t> _value0, _value1, _value2, _value3, _value4, _value10, _data_length, _error;
        std::optional<std::string> _data, _name;
        std::optional<session_info> _session_info;
    };
}


#endif //WORMS_PACKET_HPP
