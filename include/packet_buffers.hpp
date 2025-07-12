// ReSharper disable CppDFAUnreachableCode
#ifndef PACKET_BUFFERS_HPP
#define PACKET_BUFFERS_HPP

#include <vector>
#include <span>
#include <optional>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <bit>
#include <expected>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

// -----------------------------------------------------------------------------
// Packet buffer utilities for modern C++23/26 async networking.
// Provides:
//   • net::PacketWriter – owning, resizable binary buffer for outgoing packets
//   • net::PacketReader – non-owning view that tracks read offset
//   • Endian‑aware write/read helpers (LE/BE)
// -----------------------------------------------------------------------------

namespace net
{
	using byte = std::byte;

	// -----------------------------------------------------------
	// PacketWriter – append‑only binary buffer for outgoing packets.
	// -----------------------------------------------------------
	class packet_writer : std::enable_shared_from_this<packet_writer>
	{
	public:
		// Constructor
		constexpr packet_writer() noexcept = default;

		constexpr void reserve(const std::size_t bytes) { _buffer.reserve(bytes); }

		template <typename T> requires std::is_trivially_copyable_v<T>
		constexpr void write(const T& value)
		{
			const auto bytes = std::as_bytes(std::span{&value, 1});
			write_bytes(bytes);
		}

		constexpr void write_bytes(std::span<const byte> bytes)
		{
			std::ranges::copy(bytes, std::back_inserter(_buffer));
		}

		template <typename T> requires std::is_integral_v<T>
		constexpr void write_le(T v)
		{
			write(std::endian::native == std::endian::little ? v : std::byteswap(v));
		}

		template <typename T> requires std::is_integral_v<T>
		constexpr void write_be(T v)
		{
			write(std::endian::native == std::endian::big ? v : std::byteswap(v));
		}

		void write_c_string(std::string_view str)
		{
			const auto bytes = std::as_bytes(std::span{str});
			write_bytes(bytes);
			write<byte>(byte{0});
		}

		// Accessors
		[[nodiscard]] constexpr std::span<const byte> span() const noexcept { return _buffer; }
		[[nodiscard]] constexpr size_t size() const noexcept { return _buffer.size(); }
		constexpr void clear() noexcept { _buffer.clear(); }

	private:
		[[no_unique_address]] std::vector<byte> _buffer;
	};

	// -----------------------------------------------------------
	// PacketReader – span‑based cursor for incoming data.
	// -----------------------------------------------------------
	class packet_reader : std::enable_shared_from_this<packet_reader>
	{
	public:
		// Constructors with deduction guides
		constexpr explicit packet_reader(std::span<const byte> data) noexcept : _data{data}
		{
		}

		constexpr packet_reader(const byte* data, size_t size) noexcept : _data{data, size}
		{
		}

		// Core reading operations with std::expected for error handling
		template <typename T> requires std::is_trivially_copyable_v<T>
		[[nodiscard]] constexpr std::expected<T, std::error_code> read()
		{
			if (_consumed + sizeof(T) > _data.size())
			{
				return std::unexpected(std::make_error_code(std::errc::message_size));
			}

			T value;
			std::memcpy(&value, _data.data() + _consumed, sizeof(T));
			_consumed += sizeof(T);
			return value;
		}

		// Byte span reading
		[[nodiscard]] constexpr std::expected<std::span<const byte>, std::error_code>
		// ReSharper disable once CppDFAUnreachableFunctionCall
		read_bytes(const size_t len)
		{
			if (_consumed + len > _data.size())
			{
				return std::unexpected(std::make_error_code(std::errc::message_size));
			}

			auto view = _data.subspan(_consumed, len);
			_consumed += len;
			return view;
		}


		// Endian-aware integral reading
		template <typename T> requires std::is_integral_v<T>
		[[nodiscard]] constexpr std::expected<T, std::error_code> read_le()
		{
			auto v = read<T>();
			if (!v) return std::unexpected(v.error());

			if constexpr (std::endian::native == std::endian::little)
			{
				return *v;
			}
			else
			{
				return std::byteswap(*v);
			}
		}

		// Endian-aware integral reading
		template <typename T> requires std::is_integral_v<T>
		[[nodiscard]] constexpr std::expected<T, std::error_code> read_be()
		{
			auto v = read<T>();
			if (!v) return std::unexpected(v.error());

			if constexpr (std::endian::native == std::endian::big)
			{
				return *v;
			}
			else
			{
				return std::byteswap(*v);
			}
		}


		[[nodiscard]] constexpr std::optional<std::string> read_c_string()
		{
			const byte* start = _data.data() + _consumed;
			const byte* end = _data.data() + _data.size();
			const byte* null_term = std::find(start, end, byte{0});

			if (null_term == end) return std::nullopt;

			const size_t str_len = null_term - start;
			_consumed += str_len + 1; // +1 for null terminator

			// reinterpret_cast ok here: std::byte is layout-compatible with char
			return std::string{reinterpret_cast<const char*>(start), str_len};
		}

		// Not null-terminated
		[[nodiscard]] constexpr std::expected<std::string_view, std::error_code> read_string(const size_t max_len)
		{
			if (auto bytes = read_bytes(max_len))
			{
				if (const auto null_pos = std::ranges::find(*bytes, byte{0}); null_pos != bytes->end())
				{
					return std::string_view{
						reinterpret_cast<const char*>(bytes->data()),
						static_cast<unsigned long long>(std::distance(bytes->begin(), null_pos))
					};
				}
			}
			return std::unexpected(std::make_error_code(std::errc::invalid_argument));
		}

		[[nodiscard]] constexpr std::expected<std::string_view, std::error_code> read_fixed_string(const size_t size)
		{
			const auto bytes = read_bytes(size);
			if (!bytes) return std::unexpected(bytes.error());
			return std::string_view{reinterpret_cast<const char*>(bytes->data()), bytes->size()};
		}

		template <typename T, size_t N>
		[[nodiscard]] constexpr std::expected<std::array<T, N>, std::error_code> read_array()
		{
			std::array<T, N> result;
			for (auto& item : result)
			{
				if (auto value = read<T>())
				{
					item = *value;
				}
				else
				{
					return std::unexpected(value.error());
				}
			}
			return result;
		}

		// ---------------------------------------------------------------------
		// Zero‑copy struct view via std::bit_cast – returns T by value (no memcpy)
		// ---------------------------------------------------------------------
		template <typename T>
			requires std::is_trivially_copyable_v<T>
		std::expected<T, std::error_code> view_struct()
		{
			if (!can_read(sizeof(T)))
				return std::unexpected(std::make_error_code(std::errc::message_size));
			// bit_cast performs compile‑time memcpy, no alignment requirement
			T v = std::bit_cast<T>(*reinterpret_cast<const std::array<byte, sizeof(T)>*>(_data.data() + _consumed));
			_consumed += sizeof(T);
			return v;
		}

		// Utility methods
		[[nodiscard]] constexpr bool can_read(const size_t bytes) const noexcept
		{
			return _consumed + bytes <= _data.size();
		}

		[[nodiscard]] constexpr std::span<const byte> remaining() const noexcept
		{
			return _data.subspan(_consumed);
		}

		[[nodiscard]] constexpr size_t bytes_read() const noexcept
		{
			return _consumed;
		}

		[[nodiscard]] constexpr size_t bytes_remaining() const noexcept
		{
			return _data.size() - _consumed;
		}

	private:
		std::span<const byte> _data;
		std::size_t _consumed{};
	};
} // namespace net

#endif // PACKET_BUFFERS_HPP
