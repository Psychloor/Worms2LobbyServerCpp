//
// Created by blomq on 2025-07-10.
//

#ifndef SESSION_INFO_HPP
#define SESSION_INFO_HPP

#include <cstdint>

#include "nation.hpp"

namespace worms_server
{
	enum class session_type : uint8_t
	{
		room = 1,
		game = 4,
		user = 5
	};

	enum class session_access : uint8_t
	{
		public_access = 1,
		protected_access = 2,
	};

	struct session_info
	{
		uint32_t crc1;
		uint32_t crc2;
		nation nation;
		uint8_t game_version;
		uint8_t game_release;
		session_type type;
		session_access access;
		uint8_t always_one;
		uint8_t always_zero;
		uint8_t padding[35];
	};

	static constexpr bool verify_session_info(const session_info& info)
	{
		if (info.crc1 != 0x17171717 || info.crc2 != 0x02010101)
		{
			return false;
		}

		if (info.always_one != 1 || info.always_zero != 0)
		{
			return false;
		}

		return true;
	}
}

#endif //SESSION_INFO_HPP
