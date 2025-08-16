#ifndef WINDOWS_1252_HPP
#define WINDOWS_1252_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace worms_server
{
    class Windows1252
    {
    public:
        static std::string encode(const std::string_view utf8Input)
        {
            std::string result;
            result.reserve(utf8Input.size());

            for (size_t i = 0; i < utf8Input.size();)
            {
                uint32_t cp = 0;
                const size_t len = utf8ToCodepoint(utf8Input, i, cp);
                if (len == 0)
                {
                    result.push_back('?');
                    ++i;
                    continue;
                }

                i += len;

                const auto encoded = unicodeToCp1252(cp);
                result.push_back(static_cast<char>(encoded));
            }

            return result;
        }

        static std::optional<std::string> encodeStrict(const std::string_view utf8Input)
        {
            std::string result;
            result.reserve(utf8Input.size());

            for (size_t i = 0; i < utf8Input.size();)
            {
                uint32_t cp = 0;
                const size_t len = utf8ToCodepoint(utf8Input, i, cp);
                if (len == 0)
                {
                    return std::nullopt;
                }

                const auto encoded = unicodeToCp1252(cp);
                if (encoded == '?')
                {
                    return std::nullopt;
                }

                result.push_back(static_cast<char>(encoded));
                i += len;
            }

            return result;
        }

        static std::string decode(const std::string& cp1252Input)
        {
            std::string result;
            result.reserve(cp1252Input.size() * 2);

            for (const unsigned char c : cp1252Input)
            {
                const auto cp = cp1252ToUnicode(c);
                append_utf8(result, cp);
            }

            return result;
        }

    private:
        // Decode: CP1252 byte to Unicode codepoint
        static constexpr uint32_t cp1252ToUnicode(const uint8_t byte)
        {
            if (byte < 0x80)
            {
                return byte;
            }

            if (byte >= 0xA0)
            {
                return byte;
            }

            // 0x80–0x9F range
            return CP1252_TABLE.at(byte - 0x80);
        }

        // Encode: Unicode codepoint to CP1252 byte
        static constexpr uint8_t unicodeToCp1252(const uint32_t cp)
        {
            if (cp < 0x80)
            {
                return static_cast<uint8_t>(cp);
            }

            if (cp >= 0xA0 && cp <= 0xFF)
            {
                return static_cast<uint8_t>(cp);
            }

            for (size_t i = 0; i < CP1252_TABLE.size(); ++i)
            {
                if (CP1252_TABLE.at(i) == cp)
                {
                    return static_cast<uint8_t>(i + 0x80);
                }
            }

            return '?';
        }

        static void append_utf8(std::string& str, const uint32_t cp)
        {
            if (cp <= 0x7F)
            {
                str.push_back(static_cast<char>(cp));
            }
            else if (cp <= 0x7FF)
            {
                str.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                str.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp <= 0xFFFF)
            {
                str.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                str.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                str.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else
            {
                str.push_back('?');
            }
        }

        static size_t utf8ToCodepoint(const std::string_view input, const size_t pos, uint32_t& cp)
        {
            const unsigned char first = input[pos];

            if ((first & 0x80) == 0x00)
            {
                cp = first;
                return 1;
            }
            if ((first & 0xE0) == 0xC0 && pos + 1 < input.size())
            {
                cp = ((first & 0x1F) << 6) | (input[pos + 1] & 0x3F);
                return 2;
            }
            if ((first & 0xF0) == 0xE0 && pos + 2 < input.size())
            {
                cp = ((first & 0x0F) << 12) | ((input[pos + 1] & 0x3F) << 6) | (input[pos + 2] & 0x3F);
                return 3;
            }
            return 0;
        }

        // 0x80–0x9F Unicode mappings
        static constexpr std::array<uint32_t, 32> CP1252_TABLE = {
            0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026,
            0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039,
            0x0152, 0xFFFD, 0x017D, 0xFFFD, 0xFFFD, 0x2018,
            0x2019,
            0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC,
            0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E,
            0x0178
        };
    };
} // namespace worms_server

#endif // WINDOWS_1252_HPP
