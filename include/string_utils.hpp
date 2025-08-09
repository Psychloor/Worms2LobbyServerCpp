#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <algorithm>

[[nodiscard]] inline bool EqualsCaseInsensitive(const std::string_view a, const std::string_view b) {
    if (a.length() != b.length()) {
        return false;
    }

    return std::ranges::equal(std::begin(a), std::end(a), std::begin(b), std::end(b),
        [](const auto lhs, const auto rhs) { return std::tolower(lhs) == std::tolower(rhs); });
}


#endif // STRING_UTILS_HPP
