//
// Created by blomq on 2025-07-10.
//

#include "user.hpp"

#include "user_session.hpp"

worms_server::user::user(const std::shared_ptr<user_session>& session, const uint32_t id, const std::string_view name,
						 const nation nation) : _id(id), _name(name), _session_info(nation, session_type::user),
												_session(session)
{
}

uint32_t worms_server::user::get_id() const
{
	std::shared_lock lock(_mutex);
	return _id;
}

std::string_view worms_server::user::get_name() const
{
	std::shared_lock lock(_mutex);
	return _name;
}

const worms_server::session_info& worms_server::user::get_session_info() const
{
	std::shared_lock lock(_mutex);
	return _session_info;
}

uint32_t worms_server::user::get_room_id() const
{
	std::shared_lock lock(_mutex);
	return _room_id;
}

void worms_server::user::set_room_id(const uint32_t room_id)
{
	std::unique_lock lock(_mutex);
	_room_id = room_id;
}

void worms_server::user::clear_session()
{
	std::unique_lock lock(_mutex);
	_session.reset();
}

void worms_server::user::send_packet(const std::span<const std::byte>& packet) const
{
	if (!_session.expired())
	{
		if (const auto session = _session.lock(); session != nullptr)
		{
			session->send_packet(packet);
		}
	}
}

boost::asio::ip::address_v4 worms_server::user::get_address() const
{
	if (!_session.expired())
	{
		if (const auto session = _session.lock(); session != nullptr)
		{
			return session->address_v4();
		}
	}

	return {};
}
