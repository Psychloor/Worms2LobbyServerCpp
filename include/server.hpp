//
// Created by blomq on 2025-07-14.
//

#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>

using boost::asio::awaitable;
using boost::asio::use_awaitable;
using namespace boost::asio;

namespace worms_server
{
	class server {
	public:

		server(uint16_t port, size_t max_connections);

		void run(size_t thread_count);
		void stop();

		static std::atomic_uint32_t connection_count;

	private:
		awaitable<void> listener();

		uint16_t _port;
		size_t _max_connections;

		io_context _io_context;
		signal_set _signals;
		std::vector<std::jthread> _threads;
	};
}

#endif //SERVER_HPP
