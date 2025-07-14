//
// Created by blomq on 2025-07-10.
//

#ifndef USER_HPP
#define USER_HPP

#include <memory>
#include <shared_mutex>
#include <string>

#include <boost/asio.hpp>

#include "session_info.hpp"
#include "worms_packet.hpp"

#include "spdlog/spdlog.h"

namespace worms_server
{
	class user_session;

	class user
	{
	public:
		explicit user(const std::shared_ptr<user_session>& session, uint32_t id, std::string_view name, nation nation);
		~user()
		{
			spdlog::debug("User {} has been destroyed", _id);
		}

		[[nodiscard]] uint32_t get_id() const;
		[[nodiscard]] std::string_view get_name() const;
		[[nodiscard]] const session_info& get_session_info() const;
		[[nodiscard]] uint32_t get_room_id() const;
		void set_room_id(uint32_t room_id);
		void clear_session();
		void send_packet(const std::span<const std::byte>& packet) const;

		boost::asio::ip::address_v4 get_address() const;



	private:
		mutable std::shared_mutex _mutex;

		uint32_t _id;
		std::string _name;
		session_info _session_info;
		uint32_t _room_id{0};
		std::weak_ptr<user_session> _session;
	};
}

#endif //USER_HPP
