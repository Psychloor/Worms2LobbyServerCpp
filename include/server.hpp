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
	class server
	{
	public:
		server(uint16_t port, size_t max_connections);

		void run(size_t thread_count);
		void stop();

		static std::atomic_uint32_t connection_count;

	private:
		awaitable<void> listener();

		uint16_t port_;
		size_t max_connections_;

		io_context io_context_;
		signal_set signals_;
		std::vector<std::jthread> threads_;
	};
}

#endif //SERVER_HPP
