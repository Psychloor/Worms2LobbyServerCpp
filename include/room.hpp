//
// Created by blomq on 2025-07-10.
//

#ifndef ROOM_HPP
#define ROOM_HPP

#include <string>
#include <boost/asio/ip/address_v4.hpp>

#include "database.hpp"
#include "session_info.hpp"
#include "spdlog/spdlog.h"

namespace worms_server
{
	class room
	{
	public:
		explicit room(uint32_t id, std::string_view name, nation nation, boost::asio::ip::address_v4 address);
		~room()
		{
			spdlog::info("Room {} has been destroyed", _id);
		}

		[[nodiscard]] uint32_t get_id() const;
		[[nodiscard]] std::string_view get_name() const;
		[[nodiscard]] const session_info& get_session_info() const;
		[[nodiscard]] boost::asio::ip::address_v4 get_address() const;

	private:
		uint32_t _id;
		std::string _name;
		session_info _session_info;
		boost::asio::ip::address_v4 _address;
	};
}

#endif //ROOM_HPP
