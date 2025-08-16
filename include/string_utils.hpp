#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <algorithm>

[[nodiscard]] inline bool EqualsCaseInsensitive(const std::string_view a, const std::string_view b)
{
    if (a.length() != b.length())
    {
        return false;
    }

    return std::ranges::equal(std::begin(a), std::end(a), std::begin(b), std::end(b),
                              [](const auto lhs, const auto rhs) { return std::tolower(lhs) == std::tolower(rhs); });
}

namespace string_utils
{
    inline std::string FromBytes(const std::span<const net::byte>& bytes)
    {
        std::vector<char> buffer(bytes.size());
        std::ranges::transform(bytes, std::begin(buffer), [](const auto& byte) { return static_cast<char>(byte); });
        return {buffer.begin(), buffer.end()};
    }
}

#endif // STRING_UTILS_HPP
