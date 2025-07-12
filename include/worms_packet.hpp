//
// Created by blomq on 2025-06-24.
//

#ifndef WORMS_PACKET_HPP
#define WORMS_PACKET_HPP

#include <array>
#include <string>
#include <optional>
#include <cstdint>

#include "packet_buffers.hpp"

#include "session_info.hpp"

namespace worms_server
{
	enum class packet_code : uint32_t;
	enum class packet_flags : uint32_t;
}

namespace worms_server
{
	class worms_packet : std::enable_shared_from_this<worms_packet>
	{
	public:
		static constexpr size_t max_data_length = 0x200;
		static constexpr size_t max_name_length = 20;

		explicit worms_packet(packet_code code);

		worms_packet with_value0(uint32_t value);
		worms_packet with_value1(uint32_t value);
		worms_packet with_value2(uint32_t value);
		worms_packet with_value3(uint32_t value);
		worms_packet with_value4(uint32_t value);
		worms_packet with_value10(uint32_t value);
		worms_packet with_data(std::string_view data);
		worms_packet with_name(std::string_view name);
		worms_packet with_error(uint32_t error);
		worms_packet with_session_info(const session_info& session_info);

		[[nodiscard]] static std::expected<std::optional<std::shared_ptr<worms_server::worms_packet>>, std::string>
		read_from(
			net::packet_reader& reader);
		void write_to(net::packet_writer& writer) const;

		[[nodiscard]] packet_code code() const;
		[[nodiscard]] size_t data_length() const;

		void set_data_length(size_t length);

	private:
		packet_code _code;
		uint32_t _flags;
		std::optional<uint32_t> _value0, _value1, _value2, _value3, _value4, _value10, _data_length, _error;
		std::optional<std::string> _data, _name;
		std::optional<session_info> _session_info;
	};
}


#endif //WORMS_PACKET_HPP
