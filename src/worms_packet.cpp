//
// Created by blomq on 2025-06-24.
//

#include "worms_packet.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <ostream>
#include <utility>

#include "packet_code.hpp"
#include "packet_flags.hpp"
#include "windows_1251.hpp"

worms_server::worms_packet::worms_packet(const packet_code code, packet_fields fields) : _code(code), _flags(0), _fields(std::move(fields))
{
}

std::expected<std::optional<std::shared_ptr<worms_server::worms_packet>>, std::string>
worms_server::worms_packet::read_from(
	net::packet_reader& reader)
{
	if (!reader.can_read(sizeof(uint32_t) * 2))
	{
		return std::nullopt;
	}
	const uint32_t code_value = reader.read_le<uint32_t>().value();
	if (!packet_code_exists(code_value))
	{
		return std::unexpected("Unknown packet code");
	}

	const auto code = static_cast<packet_code>(code_value);
	const auto flags = reader.read_le<uint32_t>().value();
	auto packet = std::make_shared<worms_packet>(code);

	if (has_flag(flags, packet_flags::value0))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->_fields.value0 = reader.read_le<uint32_t>().value();
	}

	if (has_flag(flags, packet_flags::value1))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->_fields.value1 = (reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::value2))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->_fields.value2=(reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::value3))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->_fields.value3=(reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::value4))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->_fields.value4=(reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::value10))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
		}
		packet->_fields.value10=(reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::data_length))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}

		const auto data_length = reader.read_le<uint32_t>().value();
		if (data_length > max_data_length)
		{
			return std::unexpected("Data length is too big");
		}

		packet->set_data_length(data_length);
	}

	if (has_flag(flags, packet_flags::data))
	{
		if (!reader.can_read(packet->data_length()))
		{
			return std::nullopt;
		}

		// If bit 6 is set in Flags (and implicitly bit 5 and non-zero Data Length),
		// stores a 0-terminated Windows 1251-encoded text (like an IP string or full chat message).
		// The terminator is included in Data Length.
		const auto bytes = reader.read_bytes(packet->data_length()).value();
		const auto encoded = std::string(
			reinterpret_cast<const char*>(bytes.data()),
			bytes.size() - 1 // Subtract 1 to exclude null terminator
		);

		const std::string decoded = windows_1251::decode(encoded);

		packet->_fields.data=(decoded);
	}

	if (has_flag(flags, packet_flags::error))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->_fields.error=(reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::name))
	{
		if (!reader.can_read(worms_packet::max_name_length))
		{
			return std::nullopt;
		}
		const auto name_encoded_bytes = reader.read_bytes(worms_packet::max_name_length).value();

		// Find the first null terminator or use the whole buffer
		const auto terminator_pos = std::ranges::find(name_encoded_bytes, static_cast<std::byte>(0));
		const std::string name_encoded(
			reinterpret_cast<char const*>(name_encoded_bytes.data()),
			std::distance(name_encoded_bytes.begin(), terminator_pos)
		);

		const auto name_decoded = windows_1251::decode(name_encoded);
		packet->_fields.name=(name_decoded);
	}

	if (has_flag(flags, packet_flags::session_info))
	{
		if (!reader.can_read(50))
		{
			return std::nullopt;
		}

		const auto result = session_info::read_from(reader);
		if (!result.has_value())
		{
			return std::unexpected(result.error());
		}

		session_info info = result.value().value();
		info.game_release = 49;
		packet->_fields.session_info=(std::forward<session_info>(info));
	}

	return packet;
}

void worms_server::worms_packet::write_to(net::packet_writer& writer) const
{
	writer.write_le(static_cast<uint32_t>(_code));
	writer.write_le(get_flags_set());

	if (_fields.value0.has_value())
	{
		writer.write_le(*_fields.value0);
	}

	if (_fields.value1.has_value())
	{
		writer.write_le(*_fields.value1);
	}

	if (_fields.value2.has_value())
	{
		writer.write_le(*_fields.value2);
	}

	if (_fields.value3.has_value())
	{
		writer.write_le(*_fields.value3);
	}

	if (_fields.value4.has_value())
	{
		writer.write_le(*_fields.value4);
	}

	if (_fields.value10.has_value())
	{
		writer.write_le(*_fields.value10);
	}

	if (_fields.data.has_value())
	{
		const auto encoded = windows_1251::encode(_fields.data.value());
		const auto bytes = std::as_bytes(std::span{encoded});

		writer.write_le(static_cast<uint32_t>(bytes.size_bytes() + 1));
		writer.write_bytes(bytes);
		writer.write(0);
	}

	if (_fields.error.has_value())
	{
		writer.write_le(*_fields.error);
	}

	if (_fields.name.has_value())
	{
		const auto encoded = windows_1251::encode(_fields.name.value());
		const auto bytes = std::as_bytes(std::span{encoded});
		const auto length = std::min(bytes.size_bytes(), max_name_length);

		std::array<net::byte, max_name_length> buffer{static_cast<net::byte>(0)};
		std::copy_n(bytes.begin(), length, buffer.begin());


		writer.write_bytes(buffer);
	}

	if (_fields.session_info.has_value())
	{
		const auto& info = _fields.session_info.value();
		write_session_info(writer, info);
	}
}

worms_server::packet_code worms_server::worms_packet::code() const
{
	return _code;
}

size_t worms_server::worms_packet::data_length() const
{
	return _fields.data_length.value_or(0);
}

void worms_server::worms_packet::set_data_length(size_t length)
{
	_flags |= static_cast<uint32_t>(packet_flags::data_length);
	_fields.data_length = static_cast<uint32_t>(length);
}

const worms_server::packet_fields& worms_server::worms_packet::fields() const
{
	return _fields;
}
