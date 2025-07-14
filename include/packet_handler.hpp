//
// Created by blomq on 2025-07-14.
//

#ifndef PACKET_ROUTER_HPP
#define PACKET_ROUTER_HPP

#include <memory>
#include <boost/asio.hpp>

using boost::asio::awaitable;
using boost::asio::use_awaitable;
using namespace boost::asio;

namespace worms_server
{
	class user;
	class database;
	class worms_packet;
	enum class packet_code : std::uint32_t;

	class packet_handler final : public std::enable_shared_from_this<packet_handler>
	{
	public:
		static std::shared_ptr<packet_handler> get_instance();

		awaitable<bool> handle_packet(const std::shared_ptr<user>& client_user, const std::shared_ptr<database>& database,
									  const std::shared_ptr<worms_packet>& packet);
	};
}

#endif //PACKET_ROUTER_HPP
