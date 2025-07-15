#ifndef FRAMED_PACKET_READER_HPP
#define FRAMED_PACKET_READER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "framed_deserialization_result.hpp"
#include "packet_buffers.hpp"
#include "worms_packet.hpp"

namespace net
{
	class framed_packet_reader
	{
	public:
		explicit framed_packet_reader(const size_t initial_capacity = 1024)
		{
			buffer_.reserve(initial_capacity);
		}

		void append(const std::byte* data, const size_t length)
		{
			(void)buffer_.insert(std::end(buffer_), data, data + length);
		}

		[[nodiscard]] size_t available_bytes() const noexcept
		{
			return buffer_.size();
		}

		[[nodiscard]] std::span<const std::byte> peek() const noexcept
		{
			return {buffer_};
		}

		void reserve(const size_t capacity)
		{
			buffer_.reserve(capacity);
		}

		void clear() noexcept
		{
			buffer_.clear();
		}

		deserialization_result<worms_server::worms_packet_ptr, std::string>
		try_read_packet()
		{
			if (buffer_.empty())
			{
				return {.status = packet_parse_status::partial};
			}

			// Create a view of the buffer
			packet_reader view(buffer_);

			// Try to read a packet
			auto result = worms_server::worms_packet::read_from(view);

			// Success - consume bytes
			if (result.status == packet_parse_status::complete)
			{
				if (const size_t consumed = view.bytes_read(); consumed > 0U)
				{
					if (consumed == buffer_.size())
					{
						buffer_.clear(); // Optimize a common case
					}
					else
					{
						buffer_.erase(std::cbegin(buffer_),
						              std::cbegin(buffer_) + static_cast<
							              ptrdiff_t>(
							              consumed));
					}

					// Shrink buffer if it's too large
					if (buffer_.capacity() > 16384U && buffer_.size() < (buffer_
						.capacity() >> 2)) // Divide by 4
					{
						buffer_.shrink_to_fit();
					}
				}
			}

			return std::move(result);
		}

	private:
		std::vector<std::byte> buffer_;
	};
}

#endif //FRAMED_PACKET_READER_HPP
