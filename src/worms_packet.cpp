//
// Created by blomq on 2025-06-24.
//

#include "worms_packet.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <ostream>

#include "packet_code.hpp"
#include "packet_flags.hpp"
#include "windows_1251.hpp"

worms_server::worms_packet::worms_packet(const packet_code code) : _code(code)
{
	_flags = 0;
}

worms_server::worms_packet worms_server::worms_packet::with_value0(const uint32_t value)
{
	_flags |= static_cast<uint32_t>(packet_flags::value0);
	_value0 = value;
	return *this;
}

worms_server::worms_packet worms_server::worms_packet::with_value1(const uint32_t value)
{
	_flags |= static_cast<uint32_t>(packet_flags::value1);
	_value1 = value;
	return *this;
}

worms_server::worms_packet worms_server::worms_packet::with_value2(const uint32_t value)
{
	_flags |= static_cast<uint32_t>(packet_flags::value2);
	_value2 = value;
	return *this;
}

worms_server::worms_packet worms_server::worms_packet::with_value3(const uint32_t value)
{
	_flags |= static_cast<uint32_t>(packet_flags::value3);
	_value3 = value;
	return *this;
}

worms_server::worms_packet worms_server::worms_packet::with_value4(const uint32_t value)
{
	_flags |= static_cast<uint32_t>(packet_flags::value4);
	_value4 = value;
	return *this;
}

worms_server::worms_packet worms_server::worms_packet::with_value10(const uint32_t value)
{
	_flags |= static_cast<uint32_t>(packet_flags::value10);
	_value10 = value;
	return *this;
}

worms_server::worms_packet worms_server::worms_packet::with_data(const std::string_view data)
{
	_flags |= static_cast<uint32_t>(packet_flags::data_length);
	_flags |= static_cast<uint32_t>(packet_flags::data);

	_data_length = static_cast<uint32_t>(data.size() + 1);
	_data = std::string(data);

	return *this;
}

worms_server::worms_packet worms_server::worms_packet::with_name(const std::string_view name)
{
	_flags |= static_cast<uint32_t>(packet_flags::name);

	// std::array<uint8_t, max_name_length> name_utf8{0};
	// const auto length = std::min(name.size(), max_name_length);
	// std::copy_n(name.begin(), length, name_utf8.begin());

	if (name.size() > max_name_length)
	{
		_name = name.substr(0, max_name_length);
		return *this;
	}
	_name = name;
	return *this;
}

worms_server::worms_packet worms_server::worms_packet::with_error(uint32_t error)
{
	_flags |= static_cast<uint32_t>(packet_flags::error);
	_error = error;
	return *this;
}

worms_server::worms_packet worms_server::worms_packet::with_session_info(const session_info& session_info)
{
	_flags |= static_cast<uint32_t>(packet_flags::session_info);
	_session_info = session_info;
	return *this;
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
		packet->with_value0(reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::value1))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->with_value1(reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::value2))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->with_value2(reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::value3))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->with_value3(reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::value4))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->with_value4(reader.read_le<uint32_t>().value());
	}

	if (has_flag(flags, packet_flags::value10))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
		}
		packet->with_value10(reader.read_le<uint32_t>().value());
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

		packet->with_data(decoded);
	}

	if (has_flag(flags, packet_flags::error))
	{
		if (!reader.can_read(sizeof(uint32_t)))
		{
			return std::nullopt;
		}
		packet->with_error(reader.read_le<uint32_t>().value());
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
		packet->with_name(name_decoded);
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
		packet->with_session_info(std::forward<session_info>(info));
	}

	return packet;
}

worms_server::worms_packet worms_server::worms_packet::write_to(net::packet_writer& writer) const
{
	writer.write_le(static_cast<uint32_t>(_code));
	writer.write_le(_flags);

	if (_value0.has_value())
	{
		writer.write_le(*_value0);
	}

	if (_value1.has_value())
	{
		writer.write_le(*_value1);
	}

	if (_value2.has_value())
	{
		writer.write_le(*_value2);
	}

	if (_value3.has_value())
	{
		writer.write_le(*_value3);
	}

	if (_value4.has_value())
	{
		writer.write_le(*_value4);
	}

	if (_value10.has_value())
	{
		writer.write_le(*_value10);
	}

	if (_data.has_value())
	{
		const auto encoded = windows_1251::encode(_data.value());
		const auto bytes = std::as_bytes(std::span{encoded});

		writer.write_le(static_cast<uint32_t>(bytes.size_bytes() + 1));
		writer.write_bytes(bytes);
		writer.write(0);
	}

	if (_error.has_value())
	{
		writer.write_le(*_error);
	}

	if (_name.has_value())
	{
		const auto encoded = windows_1251::encode(_name.value());
		const auto bytes = std::as_bytes(std::span{encoded});
		const auto length = std::min(bytes.size_bytes(), max_name_length);

		std::array<net::byte, max_name_length> buffer{static_cast<net::byte>(0)};
		std::copy_n(bytes.begin(), length, buffer.begin());


		writer.write_bytes(buffer);
	}

	if (_session_info.has_value())
	{
		const auto& info = _session_info.value();
		write_session_info(writer, info);
	}

	return *this;
}

worms_server::packet_code worms_server::worms_packet::code() const
{
	return _code;
}

size_t worms_server::worms_packet::data_length() const
{
	return _data_length.value_or(0);
}

void worms_server::worms_packet::set_data_length(size_t length)
{
	_flags |= static_cast<uint32_t>(packet_flags::data_length);
	_data_length = static_cast<uint32_t>(length);
}
