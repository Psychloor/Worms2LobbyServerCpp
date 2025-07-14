//
// Created by blomq on 2025-07-10.
//

#include "room.hpp"

#include <utility>
namespace worms_server
{
	room::room(const uint32_t id, const std::string_view name, const nation nation,
							 boost::asio::ip::address_v4 address) : id_(id), name_(name),
																	session_info_{nation, session_type::room},
																	address_(std::move(address))
	{
	}

	uint32_t room::get_id() const
	{
		return id_;
	}

	std::string_view room::get_name() const
	{
		return name_;
	}

	const session_info& room::get_session_info() const
	{
		return session_info_;
	}

	boost::asio::ip::address_v4 room::get_address() const
	{
		return address_;
	}
}