//
// Created by blomq on 2025-07-10.
//

#include "user.hpp"

worms_server::user::user(const uint32_t id, const std::string_view name, const nation nation)
	: _id(id)
	  , _name(name)
	  , _session_info{nation, session_type::user}
	  , _room_id(0)
{
}

uint32_t worms_server::user::get_id() const
{
	return _id;
}

std::string_view worms_server::user::get_name() const
{
	return _name;
}

const worms_server::session_info& worms_server::user::get_session_info() const
{
	return _session_info;
}

uint32_t worms_server::user::get_room_id() const
{
	return _room_id;
}

void worms_server::user::set_room_id(const uint32_t room_id)
{
	_room_id = room_id;
}

