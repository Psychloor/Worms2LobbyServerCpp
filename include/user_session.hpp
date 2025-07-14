//
// Created by blomq on 2025-07-14.
//

#ifndef USER_SESSION_HPP
#define USER_SESSION_HPP

#include <boost/asio.hpp>

#include "concurrentqueue.hpp"
#include "packet_buffers.hpp"

namespace worms_server
{
	class room;
	class user;

	using boost::asio::awaitable;
	using boost::asio::use_awaitable;
	using namespace boost::asio;

	class user_session final : public std::enable_shared_from_this<user_session>
	{
	public:
		explicit user_session(ip::tcp::socket socket);
		~user_session();

		awaitable<void> run();

		void send_packet(const net::shared_bytes_ptr& packet);
		ip::address_v4 address_v4() const;

	private:
		awaitable<std::shared_ptr<user>> handle_login();
		awaitable<void> handle_session();

		awaitable<void> writer();

		std::atomic<bool> _is_shutting_down{false};

		ip::tcp::socket _socket;
		std::shared_ptr<user> _user;

		steady_timer _timer;
		moodycamel::ConcurrentQueue<net::shared_bytes_ptr> _packets;
	};
}

#endif //USER_SESSION_HPP
