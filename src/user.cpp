//
// Created by blomq on 2025-07-10.
//

#include "user.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>

worms_server::user::user(tcp::socket socket, const uint32_t id, const std::string_view name, const nation nation) :
	_socket(
		std::move(socket)), _id(id), _name(name), _session_info{nation, session_type::user}, _room_id(0),
	_timer(socket.get_executor())
{
	_timer.expires_at(std::chrono::steady_clock::time_point::max());
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

void worms_server::user::send_packet(const std::span<const std::byte>& packet)
{
	_packets.enqueue({packet.begin(), packet.end()});
	_timer.cancel_one();
}

void worms_server::user::start_writer()
{
	co_spawn(_socket.get_executor(), [this] { return writer(); }, boost::asio::detached);
}

boost::asio::awaitable<size_t> worms_server::user::async_receive(const boost::asio::mutable_buffer& buffer,
																 boost::system::error_code& ec)
{
	return _socket.async_receive(buffer, redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::ip::address_v4 worms_server::user::get_address() const
{
	return _socket.remote_endpoint().address().to_v4();
}

boost::asio::awaitable<void> worms_server::user::writer()
{
	try
	{
		std::vector<std::byte> current_packet;

		while (_socket.is_open())
		{
			if (_packets.try_dequeue(current_packet))
			{
				// Send packets as long as we have them
				do
				{
					co_await boost::asio::async_write(_socket,
						boost::asio::buffer(current_packet),
						boost::asio::use_awaitable);
				} while (_packets.try_dequeue(current_packet));
			}

			// Only wait if we had no packets
			boost::system::error_code ec;
			co_await _timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
		}
	}
	catch (std::exception&)
	{
	}
}
