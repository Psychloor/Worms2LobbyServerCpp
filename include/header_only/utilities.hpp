//
// Created by blomq on 2025-07-14.
//

#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <string>
#include <string_view>
#include <vector>

inline bool string_equals(std::string_view a, std::string_view b)
{
	return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](const char left, const char right)
	{
		return std::tolower(left) == std::tolower(right);
	});
}

#endif //UTILITIES_HPP
