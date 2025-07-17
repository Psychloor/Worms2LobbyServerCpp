#ifndef WORMS_PACKET_HPP
#define WORMS_PACKET_HPP

#include <cstdint>
#include <optional>
#include <string>

#include "packet_buffers.hpp"
#include "packet_code.hpp"
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

		constexpr uint32_t get_flags_from_fields() const;

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
