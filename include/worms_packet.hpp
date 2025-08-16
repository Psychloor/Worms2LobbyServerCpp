#ifndef WORMS_PACKET_HPP
#define WORMS_PACKET_HPP

#include <cstdint>
#include <optional>
#include <string>

#include "framed_packet_reader.hpp"
#include "packet_buffers.hpp"
#include "packet_code.hpp"
#include "session_info.hpp"

namespace worms_server {
    class WormsPacket;
    enum class PacketCode : uint16_t;
    enum class PacketFlags : uint16_t;
} // namespace worms_server

namespace worms_server {
    using WormsPacketPtr = std::shared_ptr<WormsPacket>;

    struct PacketFields {
        std::optional<uint32_t> value0, value1, value2, value3, value4, value10, dataLength;
        std::optional<std::string> name;
        std::optional<std::string> data;
        std::optional<SessionInfo> info;
        std::optional<uint32_t> error;
    };

    class WormsPacket : public std::enable_shared_from_this<WormsPacket> {
    public:
        static constexpr size_t MAX_DATA_LENGTH = 0x200;
        static constexpr size_t MAX_NAME_LENGTH = 20;
        static std::atomic<bool> useWindows1252Encoding;

        static net::shared_bytes_ptr freeze(PacketCode code, PacketFields fields = {});

        explicit WormsPacket(PacketCode code, PacketFields fields = {});

        [[nodiscard]] static net::deserialization_result<WormsPacketPtr, std::string> readFrom(
            net::packet_reader& reader);

        [[nodiscard]] PacketCode code() const;
        [[nodiscard]] size_t dataLength() const;

        void setDataLength(size_t length);

        [[nodiscard]] const PacketFields& fields() const;

        constexpr uint32_t getFlagsFromFields() const;

        template <PacketCode Code>
        static const net::shared_bytes_ptr& getCachedPacket() {
            static const auto packet = freeze(Code);
            return packet;
        }


        static const net::shared_bytes_ptr& getListEndPacket() {
            return getCachedPacket<PacketCode::ListEnd>();
        }

    private:
        static inline std::string encodeString(const std::string& input);
        static inline std::string decodeString(const std::string& input);

        void writeTo(net::packet_writer& writer) const;
        PacketCode code_;
        uint32_t flags_;
        PacketFields fields_;
    };
} // namespace worms_server


#endif // WORMS_PACKET_HPP
