//
// Created by blomq on 2025-07-10.
//

#ifndef USER_HPP
#define USER_HPP

#include <memory>
#include <string>

#include "session_info.hpp"

namespace worms_server
{
	class user final : std::enable_shared_from_this<user>
	{
	public:
		explicit user(uint32_t id, std::string_view name, nation nation);

		[[nodiscard]] uint32_t get_id() const;
		[[nodiscard]] std::string_view get_name() const;
		[[nodiscard]] const session_info& get_session_info() const;
		[[nodiscard]] uint32_t get_room_id() const;
		void set_room_id(uint32_t room_id);

	private:
		uint32_t _id;
		std::string _name;
		session_info _session_info;
		uint32_t _room_id;
	};
}

#endif //USER_HPP
