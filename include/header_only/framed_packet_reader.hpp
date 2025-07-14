//
// Created by blomq on 2025-07-14.
//

#ifndef FRAMED_PACKET_READER_HPP
#define FRAMED_PACKET_READER_HPP
#include <cstddef>
#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "packet_buffers.hpp"
#include "worms_packet.hpp"

namespace net
{
	using parse_result =
	std::expected<std::optional<std::shared_ptr<worms_server::worms_packet>>, std::string>;

	class framed_packet_reader
	{
	public:
		void append(const std::byte* data, const size_t length)
		{
			_buffer.insert(_buffer.end(), data, data + length);
		}

		parse_result try_read_packet()
		{
			if (_buffer.empty())
				return std::optional<std::shared_ptr<worms_server::worms_packet>>{}; // no data, no packet

			packet_reader view(_buffer);
			auto result = worms_server::worms_packet::read_from(view);
			if (!result) // protocol error
				return std::unexpected(result.error());

			const auto& optional_packet = *result; // optional<packet>
			if (!optional_packet) // incomplete, need more
				return optional_packet;

			// success – consume bytes and return
			const std::size_t consumed = view.bytes_read();
			_buffer.erase(_buffer.begin(), _buffer.begin() + static_cast<ptrdiff_t>(consumed));
			return optional_packet;
		}

	private:
		std::vector<std::byte> _buffer;
	};
}

#endif //FRAMED_PACKET_READER_HPP
