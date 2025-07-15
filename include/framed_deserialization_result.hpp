//
// Created by blomq on 2025-07-15.
//

#ifndef FRAMED_DESERIALIZATION_RESULT_HPP
#define FRAMED_DESERIALIZATION_RESULT_HPP

#include <optional>

namespace net
{
    enum class packet_parse_status
    {
        complete,
        partial,
        error,
    };

    template <typename T, typename E>
    struct deserialization_result
    {
        packet_parse_status status;
        std::optional<T> data;
        std::optional<E> error;
    };
}

#endif //FRAMED_DESERIALIZATION_RESULT_HPP
