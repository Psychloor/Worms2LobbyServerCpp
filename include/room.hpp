//
// Created by blomq on 2025-07-10.
//

#ifndef ROOM_HPP
#define ROOM_HPP

#include <boost/asio/ip/address_v4.hpp>
#include <string>

#include "session_info.hpp"
#include "spdlog/spdlog.h"

namespace worms_server
{
	class room
	{
	public:
		explicit room(uint32_t id, std::string_view name, nation nation,
					  boost::asio::ip::address_v4 address);

		~room()
		{
			spdlog::debug("Room {} has been destroyed", id_);
		}

		[[nodiscard]] uint32_t get_id() const;
		[[nodiscard]] std::string_view get_name() const;
		[[nodiscard]] const session_info& get_session_info() const;
		[[nodiscard]] boost::asio::ip::address_v4 get_address() const;

	private:
		uint32_t id_;
		std::string name_;
		session_info session_info_;
		boost::asio::ip::address_v4 address_;
	};
}

#endif //ROOM_HPP
