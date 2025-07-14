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
		explicit framed_packet_reader(const size_t initial_capacity = 1024)
		{
			_buffer.reserve(initial_capacity);
		}

		void append(std::span<const std::byte> data)
		{
			_buffer.insert(_buffer.end(), data.begin(), data.end());
		}

		void append(const std::byte* data, const size_t length)
		{
			_buffer.insert(_buffer.end(), data, data + length);
		}

		[[nodiscard]] size_t available_bytes() const noexcept
		{
			return _buffer.size();
		}

		[[nodiscard]] std::span<const std::byte> peek() const noexcept
		{
			return std::span<const std::byte>(_buffer);
		}

		void reserve(size_t capacity)
		{
			_buffer.reserve(capacity);
		}

		void clear() noexcept
		{
			_buffer.clear();
		}

		parse_result try_read_packet()
		{
			if (_buffer.empty())
			{
				return std::nullopt;
			}

			// Create a view of the buffer
			packet_reader view(_buffer);

			// Try to read a packet
			auto result = worms_server::worms_packet::read_from(view);
			if (!result)
			{
				return std::unexpected(result.error());
			}

			// Need more data
			const auto& optional_packet = *result;
			if (!optional_packet)
			{
				return std::nullopt;
			}

			// Success - consume bytes
			if (const size_t consumed = view.bytes_read(); consumed > 0)
			{
				if (consumed == _buffer.size())
				{
					_buffer.clear(); // Optimize a common case
				}
				else
				{
					_buffer.erase(_buffer.begin(),
								  _buffer.begin() + static_cast<ptrdiff_t>(consumed));
				}

				// Shrink buffer if it's too large
				if (_buffer.capacity() > 16384 && _buffer.size() < _buffer.capacity() / 4)
				{
					_buffer.shrink_to_fit();
				}
			}

			return optional_packet;
		}

	private:
		std::vector<std::byte> _buffer;
	};
}

#endif //FRAMED_PACKET_READER_HPP
