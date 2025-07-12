//
// Created by blomq on 2025-07-10.
//

#ifndef USER_HPP
#define USER_HPP

#include <deque>
#include <memory>
#include <shared_mutex>
#include <string>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>

#include "session_info.hpp"
#include "worms_packet.hpp"

namespace worms_server
{
	using tcp = boost::asio::ip::tcp;

	class user final : std::enable_shared_from_this<user>
	{
	public:
		explicit user(tcp::socket socket, uint32_t id, std::string_view name, nation nation);

		[[nodiscard]] uint32_t get_id() const;
		[[nodiscard]] std::string_view get_name() const;
		[[nodiscard]] const session_info& get_session_info() const;
		[[nodiscard]] uint32_t get_room_id() const;
		void set_room_id(uint32_t room_id);

		void send_packet(const std::span<const std::byte>& packet);
		void start_writer();


	private:
		mutable std::shared_mutex _mutex;

		boost::asio::awaitable<void> writer();

		tcp::socket _socket;
		uint32_t _id;
		std::string _name;
		session_info _session_info;
		uint32_t _room_id;

		boost::asio::steady_timer _timer;
		std::deque<std::span<const std::byte>> _packets;
	};
}

#endif //USER_HPP
