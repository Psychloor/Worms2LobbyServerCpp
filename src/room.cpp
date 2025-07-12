//
// Created by blomq on 2025-07-10.
//

#include "room.hpp"

#include <utility>

worms_server::room::room(const uint32_t id, const std::string_view name, const nation nation,
						 boost::asio::ip::address_v4 address) : _id(id), _name(name),
																_session_info{nation, session_type::room},
																_address(std::move(address))
{
}

uint32_t worms_server::room::get_id() const
{
	return _id;
}

std::string_view worms_server::room::get_name() const
{
	return _name;
}

const worms_server::session_info& worms_server::room::get_session_info() const
{
	return _session_info;
}

boost::asio::ip::address_v4 worms_server::room::get_address() const
{
	return _address;
}
