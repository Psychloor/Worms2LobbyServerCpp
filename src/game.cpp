//
// Created by blomq on 2025-07-10.
//

#include "game.hpp"

#include <utility>

worms_server::game::game(const uint32_t id, const std::string_view name, const nation nation, const uint32_t room_id,
						 boost::asio::ip::address_v4 address, const session_access access) : _id(id), _name(name),
	_session_info{nation, session_type::game, access}, _address(std::move(address)), _room_id(room_id)
{
}

uint32_t worms_server::game::get_id() const
{
	return _id;
}

std::string_view worms_server::game::get_name() const
{
	return _name;
}

const worms_server::session_info& worms_server::game::get_session_info() const
{
	return _session_info;
}

boost::asio::ip::address_v4 worms_server::game::get_address() const
{
	return _address;
}

uint32_t worms_server::game::get_room_id() const
{
	return _room_id;
}
