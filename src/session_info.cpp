//
// Created by blomq on 2025-07-17.
//

#include "session_info.hpp"

namespace worms_server
{
	session_info::session_info(const worms_server::nation nation,
		const session_type type, const session_access access)
	{
		this->nation = nation;
		this->type = type;
		this->access = access;

		crc1 = 0x17171717;
		crc2 = 0x02010101;
		always_one = 1;
		always_zero = 0;
		game_release = 49;
		game_version = 49;

		padding.fill(static_cast<net::byte>(0));
	}

	void session_info::write_to(net::packet_writer& writer) const
	{
		writer.write_le(crc1);
		writer.write_le(crc2);
		writer.write_le(static_cast<uint8_t>(nation));
		writer.write_le(game_version);
		writer.write_le(game_release);
		writer.write_le(static_cast<uint8_t>(type));
		writer.write_le(static_cast<uint8_t>(access));
		writer.write_le(always_one);
		writer.write_le(always_zero);
		writer.write(padding);
	}

	net::deserialization_result<session_info, std::string>
		session_info::read_from(net::packet_reader& reader)
	{
		session_info info;

		info.crc1 = reader.read_le<uint32_t>().value();
		info.crc2 = reader.read_le<uint32_t>().value();
		info.nation = static_cast<worms_server::nation>(reader.read_le<
			uint8_t>().value());
		info.game_version = reader.read_le<uint8_t>().value();
		info.game_release = reader.read_le<uint8_t>().value();
		info.type = static_cast<session_type>(reader.read_le<uint8_t>().
			value());
		info.access = static_cast<session_access>(reader.read_le<uint8_t>().
			value());
		info.always_one = reader.read_le<uint8_t>().value();
		info.always_zero = reader.read_le<uint8_t>().value();

		const auto padding_bytes = reader.read_bytes(padding_size).value();
		std::ranges::copy(padding_bytes, std::begin(info.padding));

		if (!verify_session_info(info))
		{
			return {
				.status = net::packet_parse_status::error,
				.error = "Invalid session info"
			};
		}

		return {
			.status = net::packet_parse_status::complete,
			.data = std::make_optional(info)
		};
	}

	bool session_info::verify_session_info(const session_info& info)
	{		static constexpr uint32_t expected_crc2 =
			std::endian::native == std::endian::little
				? 0x02010101U
				: std::byteswap(0x02010101U);

		if (info.crc1 != 0x17171717U || info.crc2 != expected_crc2)
		{
			spdlog::error("CRC Missmatch - CRC1: {} CRC2: {}",
						  info.crc1,
						  info.crc2);
			return false;
		}

		if (info.always_one != 1 || info.always_zero != 0)
		{
			spdlog::error(
				"Always one/zero mismatch - Always one: {} Always zero: {}",
				info.always_one,
				info.always_zero);
			return false;
		}

		const auto result = std::ranges::find_if_not(
			info.padding,
			[](const auto& value) { return value == net::byte{0}; });

		if (result != std::end(info.padding))
		{
			spdlog::error("Padding contains non-zero bytes");
			return false;
		}

		return true;
	};

}