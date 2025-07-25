#ifndef WINDOWS_1251_HPP
#define WINDOWS_1251_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace worms_server
{
    class windows_1251
    {
    public:
        // UTF-8 → Windows-1251 (lossy)
        static std::string encode(const std::string_view utf8_input)
        {
            std::string result;
            result.reserve(utf8_input.size());

            for (size_t i = 0; i < utf8_input.length();)
            {
                uint32_t codepoint = 0;
                const size_t len = utf8_to_codepoint(utf8_input, i, codepoint);

                if (len == 0)
                {
                    result.push_back('?');
                    i += 1;
                    continue;
                }

                i += len;
                result.push_back(
                    static_cast<char>(unicode_to_windows1251(codepoint)));
            }

            return result;
        }

        // UTF-8 → Windows-1251 (strict)
        static std::optional<std::string>
        encode_strict(const std::string_view utf8_input)
        {
            std::string result;
            result.reserve(utf8_input.size());

            for (size_t i = 0; i < utf8_input.length();)
            {
                uint32_t codepoint = 0;
                const size_t len = utf8_to_codepoint(utf8_input, i, codepoint);
                if (len == 0)
                    return std::nullopt;

                const uint8_t ch = unicode_to_windows1251(codepoint);
                if (ch == '?')
                    return std::nullopt;

                result.push_back(static_cast<char>(ch));
                i += len;
            }

            return result;
        }

        // Windows-1251 → UTF-8
        static std::string decode(const std::string& win1251_input)
        {
            std::string result;
            result.reserve(win1251_input.size() *
                           2); // UTF-8 may need more space

            for (const unsigned char c : win1251_input)
            {
                const uint32_t unicode = windows1251_to_unicode(c);
                append_utf8(result, unicode);
            }

            return result;
        }

    private:
        static constexpr uint8_t unicode_to_windows1251(const uint32_t unicode)
        {
            if (unicode < 0x80)
                return static_cast<uint8_t>(unicode);

            // Cyrillic range
            if (unicode >= 0x0410 && unicode <= 0x044F)
                return static_cast<uint8_t>(unicode - 0x0410 + 0xC0);

            // Special characters
            switch (unicode)
            {
            case 0x2116:
                return 0xB9; // №
            case 0x0401:
                return 0xA8; // Ё
            case 0x0451:
                return 0xB8; // ё
            default:
                return '?'; // Unsupported
            }
        }

        static uint32_t windows1251_to_unicode(const uint8_t c)
        {
            constexpr static std::array<uint32_t, 128> table = {
                0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
                0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
                0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
                0x0098, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
                0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
                0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
                0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
                0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
                0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
                0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
                0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
                0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
                0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
                0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
                0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
                0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F};

            return (c < 0x80) ? c : table[c - 0x80];
        }

        static size_t utf8_to_codepoint(const std::string_view input,
                                        const size_t pos, uint32_t& codepoint)
        {
            const unsigned char first = input[pos];

            if ((first & 0x80) == 0x00)
            {
                codepoint = first;
                return 1;
            }

            if ((first & 0xE0) == 0xC0 && pos + 1 < input.size())
            {
                codepoint = ((first & 0x1F) << 6) | (input[pos + 1] & 0x3F);
                return 2;
            }

            if ((first & 0xF0) == 0xE0 && pos + 2 < input.size())
            {
                codepoint = ((first & 0x0F) << 12) |
                    ((input[pos + 1] & 0x3F) << 6) | (input[pos + 2] & 0x3F);
                return 3;
            }

            return 0; // Invalid or unsupported (e.g., 4-byte UTF-8)
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
                str.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
                str.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                str.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else
            {
                str.push_back('?'); // Not supported in Windows-1251
            }
        }
    };
} // namespace worms_server

#endif // WINDOWS_1251_HPP
