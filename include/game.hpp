//
// Created by blomq on 2025-07-10.
//

#ifndef GAME_HPP
#define GAME_HPP

#include <boost/asio/ip/address_v4.hpp>

#include "session_info.hpp"

namespace worms_server
{
	class game final : std::enable_shared_from_this<game>
	{
	public:
		explicit game(uint32_t id, std::string_view name, nation nation, uint32_t room_id,
					  boost::asio::ip::address_v4 address, session_access access);

		[[nodiscard]] uint32_t get_id() const;
		[[nodiscard]] std::string_view get_name() const;
		[[nodiscard]] const session_info& get_session_info() const;
		[[nodiscard]] boost::asio::ip::address_v4 get_address() const;
		[[nodiscard]] uint32_t get_room_id() const;

	private:
		uint32_t _id;
		std::string _name;
		session_info _session_info;
		boost::asio::ip::address_v4 _address;
		uint32_t _room_id;
	};
}

#endif //GAME_HPP
