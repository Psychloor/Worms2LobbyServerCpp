﻿// ReSharper disable CppDFAUnreachableCode
#ifndef PACKET_BUFFERS_HPP
#define PACKET_BUFFERS_HPP

#include <bit>
#include <cstddef>
#include <cstring>
#include <expected>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

// -----------------------------------------------------------------------------
// Packet buffer utilities for modern C++23/26 async networking.
// Provides:
//   • net::packet_writer – owning, resizable binary buffer for outgoing packets
//   • net::packet_reader – non-owning view that tracks read offset
//   • Endian‑aware write/read helpers (LE/BE)
// -----------------------------------------------------------------------------

namespace net
{
    struct shared_bytes;
    using byte = std::byte;
    using shared_bytes_ptr = std::shared_ptr<shared_bytes>;

    // Base bytes struct with CRTP pattern
    template <typename Derived>
    struct bytes_base : std::enable_shared_from_this<Derived>
    {
        virtual ~bytes_base() = default;
        using byte = std::byte;

        [[nodiscard]] std::span<const byte> view() const noexcept
        {
            return {data(), size()};
        }

        // Convert to shared_bytes_ptr
        [[nodiscard]] shared_bytes_ptr to_shared()
        {
            return std::static_pointer_cast<shared_bytes>(
                this->shared_from_this());
        }

        // Helper for view comparison
        [[nodiscard]] bool view_equals(const bytes_base& other) const noexcept
        {
            return size() == other.size() &&
                std::equal(data(), data() + size(), other.data());
        }


        [[nodiscard]] virtual const byte* data() const noexcept = 0;
        [[nodiscard]] virtual size_t size() const noexcept = 0;
    };

    // Immutable shared bytes implementation
    struct shared_bytes final : bytes_base<shared_bytes>
    {
        explicit shared_bytes(std::vector<byte>&& data) : data_(std::move(data))
        {
        }

        [[nodiscard]] const byte* data() const noexcept override
        {
            return data_.data();
        }

        [[nodiscard]] size_t size() const noexcept override
        {
            return data_.size();
        }

    private:
        std::vector<byte> data_;
    };


    // Helper for creating shared bytes
    template <typename... Args>
    [[nodiscard]] auto make_shared_bytes(Args&&... args)
    {
        return std::make_shared<shared_bytes>(std::forward<Args>(args)...);
    }


    // -----------------------------------------------------------
    // PacketWriter – append‑only binary buffer for outgoing packets.
    // -----------------------------------------------------------
    class packet_writer : std::enable_shared_from_this<packet_writer>
    {
    public:
        // Constructor
        constexpr packet_writer() noexcept = default;

        constexpr void reserve(const size_t bytes) { buffer_.reserve(bytes); }

        template <typename T>
            requires std::is_trivially_copyable_v<T>
        constexpr void write(const T& value)
        {
            const auto bytes = std::as_bytes(std::span{&value, 1});
            write_bytes(bytes);
        }

        constexpr void write_bytes(std::span<const byte> bytes)
        {
            buffer_.insert(std::end(buffer_), bytes.begin(), bytes.end());
        }

        template <typename T>
            requires std::is_integral_v<T>
        constexpr void write_le(T v)
        {
            write(std::endian::native == std::endian::little
                      ? v
                      : std::byteswap(v));
        }

        template <typename T>
            requires std::is_integral_v<T>
        constexpr void write_be(T v)
        {
            write(std::endian::native == std::endian::big ? v
                                                          : std::byteswap(v));
        }

        // Accessors
        [[nodiscard]] constexpr std::span<const byte> span() const noexcept
        {
            return buffer_;
        }

        [[nodiscard]] constexpr size_t size() const noexcept
        {
            return buffer_.size();
        }

        // Transfer ownership of the buffer to shared_bytes for downstream async
        // use
        [[nodiscard]] shared_bytes_ptr to_shared() &&
        {
            return make_shared_bytes(std::move(buffer_));
        }

        // Efficient append from shared_bytes
        void append_bytes(const shared_bytes& bytes)
        {
            write_bytes(bytes.view());
        }


        constexpr void clear() noexcept { buffer_.clear(); }

    private:
        [[no_unique_address]] std::vector<byte> buffer_;
    };

    // -----------------------------------------------------------
    // PacketReader – span‑based cursor for incoming data.
    // -----------------------------------------------------------
    class packet_reader : std::enable_shared_from_this<packet_reader>
    {
    public:
        // Constructors with deduction guides
        constexpr explicit packet_reader(
            const std::span<const std::byte> data) noexcept : data_(data)
        {
        }

        // Core reading operations with std::expected for error handling
        template <typename T>
            requires std::is_trivially_copyable_v<T>
        [[nodiscard]] constexpr std::expected<T, std::error_code> read()
        {
            if (consumed_ + sizeof(T) > data_.size())
            {
                return std::unexpected(
                    std::make_error_code(std::errc::message_size));
            }

            T value;
            std::memcpy(&value, data_.data() + consumed_, sizeof(T));
            consumed_ += sizeof(T);
            return value;
        }

        // Byte span reading
        [[nodiscard]] constexpr std::expected<std::span<const byte>,
                                              std::error_code>
        // ReSharper disable once CppDFAUnreachableFunctionCall
        read_bytes(const size_t len)
        {
            if (consumed_ + len > data_.size())
            {
                return std::unexpected(
                    std::make_error_code(std::errc::message_size));
            }

            auto view = data_.subspan(consumed_, len);
            consumed_ += len;
            return view;
        }


        // Endian-aware integral reading
        template <typename T>
            requires std::is_integral_v<T>
        [[nodiscard]] constexpr std::expected<T, std::error_code> read_le()
        {
            auto v = read<T>();
            if (!v)
                return std::unexpected(v.error());

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
        template <typename T>
            requires std::is_integral_v<T>
        [[nodiscard]] constexpr std::expected<T, std::error_code> read_be()
        {
            auto v = read<T>();
            if (!v)
                return std::unexpected(v.error());

            if constexpr (std::endian::native == std::endian::big)
            {
                return *v;
            }
            else
            {
                return std::byteswap(*v);
            }
        }

        [[nodiscard]] std::expected<std::string, std::error_code>
        read_c_string()
        {
            auto rem = remaining();
            const auto null_pos = std::ranges::find(rem, std::byte{0});

            if (null_pos == rem.end())
            {
                // No null terminator in the current buffer
                return std::unexpected(
                    std::make_error_code(std::errc::invalid_argument));
            }

            const auto offset = static_cast<size_t>(null_pos - rem.begin());

            std::string str{reinterpret_cast<const char*>(rem.data()), offset};

            consumed_ += offset + 1; // +1 to skip the null terminator
            return std::move(str);
        }

        [[nodiscard]] constexpr std::expected<std::string, std::error_code>
        read_c_string_limited(const size_t max_len)
        {
            auto rem = remaining();
            const auto len = std::min(rem.size(), max_len);

            const auto null_pos =
                std::ranges::find(rem.first(len), std::byte{0});
            if (null_pos == rem.begin() + static_cast<ptrdiff_t>(len))
            {
                return std::unexpected(
                    std::make_error_code(std::errc::invalid_argument));
            }

            const auto offset = static_cast<size_t>(null_pos - rem.begin());

            std::string str{reinterpret_cast<const char*>(rem.data()), offset};

            consumed_ += offset + 1;
            return std::move(str);
        }

        // Not null-terminated
        [[nodiscard]] constexpr std::expected<std::string, std::error_code>
        read_string(const size_t max_len)
        {
            if (auto bytes = read_bytes(max_len))
            {
                if (const auto null_pos = std::ranges::find(*bytes, byte{0});
                    null_pos != bytes->end())
                {
                    return std::string{
                        reinterpret_cast<const char*>(bytes->data()),
                        static_cast<unsigned long long>(
                            std::distance(bytes->begin(), null_pos))};
                }
            }
            return std::unexpected(
                std::make_error_code(std::errc::invalid_argument));
        }

        [[nodiscard]] constexpr std::expected<std::string_view, std::error_code>
        read_fixed_string(const size_t size)
        {
            const auto bytes = read_bytes(size);
            if (!bytes)
                return std::unexpected(bytes.error());
            return std::string_view{
                reinterpret_cast<const char*>(bytes->data()), bytes->size()};
        }


        // ---------------------------------------------------------------------
        // Zero‑copy struct view via std::bit_cast – returns T by value (no
        // memcpy)
        // ---------------------------------------------------------------------
        template <typename T>
            requires std::is_trivially_copyable_v<T>
        std::expected<T, std::error_code> view_struct()
        {
            if (!can_read(sizeof(T)))
                return std::unexpected(
                    std::make_error_code(std::errc::message_size));
            // bit_cast performs compile‑time memcpy, no alignment requirement
            std::array<byte, sizeof(T)> temp;
            std::memcpy(temp.data(), data_.data() + consumed_, sizeof(T));
            T v = std::bit_cast<T>(temp);
            consumed_ += sizeof(T);
            return v;
        }

        // Utility methods
        [[nodiscard]] constexpr bool can_read(const size_t bytes) const noexcept
        {
            return consumed_ + bytes <= data_.size_bytes();
        }

        [[nodiscard]] constexpr std::span<const byte> remaining() const noexcept
        {
            return std::span{std::begin(data_) +
                                 static_cast<ptrdiff_t>(consumed_),
                             data_.size_bytes() - consumed_};
        }

        [[nodiscard]] constexpr size_t bytes_read() const noexcept
        {
            return consumed_;
        }

        [[nodiscard]] constexpr size_t bytes_remaining() const noexcept
        {
            return data_.size_bytes() - consumed_;
        }

        // Reset the reader to the beginning of the buffer
        constexpr void reset() noexcept { consumed_ = 0; }

    private:
        std::span<const byte> data_;
        size_t consumed_ = 0;
    };
} // namespace net

#endif // PACKET_BUFFERS_HPP
