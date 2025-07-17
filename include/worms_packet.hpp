#ifndef WORMS_PACKET_HPP
#define WORMS_PACKET_HPP

#include <cstdint>
#include <optional>
#include <string>

#include "framed_deserialization_result.hpp"
#include "packet_buffers.hpp"
#include "packet_code.hpp"
#include "packet_flags.hpp"
#include "session_info.hpp"

namespace worms_server
{
	class worms_packet;
	enum class packet_code : uint32_t;
	enum class packet_flags : uint32_t;
}

namespace worms_server
{
	using worms_packet_ptr = std::shared_ptr<worms_packet>;

	struct packet_fields
	{
		std::optional<uint32_t> value0, value1, value2, value3, value4, value10,
								data_length;
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

		static net::shared_bytes_ptr freeze(packet_code code,
											packet_fields fields = {});

		explicit worms_packet(packet_code code, packet_fields fields = {});

		[[nodiscard]] static net::deserialization_result<
			worms_packet_ptr, std::string>
		read_from(net::packet_reader& reader);

		[[nodiscard]] packet_code code() const;
		[[nodiscard]] size_t data_length() const;

		void set_data_length(size_t length);

		[[nodiscard]] const packet_fields& fields() const;

		constexpr auto get_flags_from_fields(this const auto& self)
		{
			uint32_t flags = 0;
			if (self.fields_.value0)
				flags |= static_cast<uint32_t>(
					packet_flags::value0);
			if (self.fields_.value1)
				flags |= static_cast<uint32_t>(
					packet_flags::value1);
			if (self.fields_.value2)
				flags |= static_cast<uint32_t>(
					packet_flags::value2);
			if (self.fields_.value3)
				flags |= static_cast<uint32_t>(
					packet_flags::value3);
			if (self.fields_.value4)
				flags |= static_cast<uint32_t>(
					packet_flags::value4);
			if (self.fields_.value10)
				flags |= static_cast<uint32_t>(
					packet_flags::value10);
			if (self.fields_.data_length || self.fields_.data)
				flags |=
					static_cast<uint32_t>(packet_flags::data_length);
			if (self.fields_.data)
				flags |= static_cast<uint32_t>(
					packet_flags::data);
			if (self.fields_.error)
				flags |= static_cast<uint32_t>(
					packet_flags::error);
			if (self.fields_.name)
				flags |= static_cast<uint32_t>(
					packet_flags::name);
			if (self.fields_.session_info)
				flags |= static_cast<uint32_t>(
					packet_flags::session_info);
			return flags;
		}

		template <packet_code Code>
		static const net::shared_bytes_ptr& get_cached_packet()
		{
			static const auto packet = freeze(Code);
			return packet;
		}


		static const net::shared_bytes_ptr& get_list_end_packet()
		{
			return get_cached_packet<packet_code::list_end>();
		}

	private:
		void write_to(net::packet_writer& writer) const;
		packet_code code_;
		uint32_t flags_;
		packet_fields fields_;
	};
}


#endif //WORMS_PACKET_HPP
