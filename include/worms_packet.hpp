//
// Created by blomq on 2025-06-24.
//

#ifndef WORMS_PACKET_HPP
#define WORMS_PACKET_HPP

#include <string>
#include <optional>
#include <cstdint>

#include "packet_flags.hpp"
#include "session_info.hpp"
#include "header_only/packet_buffers.hpp"

namespace worms_server
{
	enum class packet_code : uint32_t;
	enum class packet_flags : uint32_t;
}

namespace worms_server
{
	struct packet_fields
	{
		std::optional<uint32_t> value0, value1, value2, value3, value4, value10, data_length;
		std::optional<std::string> name;
		std::optional<std::string> data;
		std::optional<session_info> session_info;
		std::optional<uint32_t> error;
	};

	class worms_packet : std::enable_shared_from_this<worms_packet>
	{
	public:
		static constexpr size_t max_data_length = 0x200;
		static constexpr size_t max_name_length = 20;

		explicit worms_packet(packet_code code, packet_fields fields = {});

		[[nodiscard]] static std::expected<std::optional<std::shared_ptr<worms_server::worms_packet>>, std::string>
		read_from(
			net::packet_reader& reader);
		void write_to(net::packet_writer& writer) const;

		[[nodiscard]] packet_code code() const;
		[[nodiscard]] size_t data_length() const;

		void set_data_length(size_t length);

		[[nodiscard]] const packet_fields& fields() const;
		void dump_log() const;

		/*template <packet_code Code, typename... Args>
		[[nodiscard]]
		static worms_packet make_packet(Args&&... args)
		{
			return worms_packet(Code, packet_fields{std::forward<Args>(args)...});
		}*/

	private:
		constexpr uint32_t get_flags_set() const;

		packet_code _code;
		uint32_t _flags;
		packet_fields _fields;
	};

	constexpr uint32_t worms_packet::get_flags_set() const
	{
		uint32_t flags = 0;
		if (_fields.value0) flags |= static_cast<uint32_t>(packet_flags::value0);
		if (_fields.value1) flags |= static_cast<uint32_t>(packet_flags::value1);
		if (_fields.value2) flags |= static_cast<uint32_t>(packet_flags::value2);
		if (_fields.value3) flags |= static_cast<uint32_t>(packet_flags::value3);
		if (_fields.value4) flags |= static_cast<uint32_t>(packet_flags::value4);
		if (_fields.value10) flags |= static_cast<uint32_t>(packet_flags::value10);
		if (_fields.data_length || _fields.data) flags |= static_cast<uint32_t>(packet_flags::data_length);
		if (_fields.data) flags |= static_cast<uint32_t>(packet_flags::data);
		if (_fields.error) flags |= static_cast<uint32_t>(packet_flags::error);
		if (_fields.name) flags |= static_cast<uint32_t>(packet_flags::name);
		if (_fields.session_info) flags |= static_cast<uint32_t>(packet_flags::session_info);
		return flags;
	}
}


#endif //WORMS_PACKET_HPP
