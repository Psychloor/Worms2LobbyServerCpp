#ifndef WINDOWS_1251_HPP
#define WINDOWS_1251_HPP

#include <array>
#include <cstdint>
#include <string>

namespace worms_server
{
    class windows_1251
    {
    public:
        static std::string encode(const std::string& utf8_input)
        {
            std::string result;
            result.reserve(utf8_input.length());

            for (size_t i = 0; i < utf8_input.length();)
            {
                uint32_t codepoint;
                const size_t len = utf8_to_codepoint(utf8_input, i, codepoint);
                i += len;

                // Find the Windows-1251 equivalent
                const uint8_t win1251_char = unicode_to_windows1251(codepoint);
                result.push_back(static_cast<char>(win1251_char));
            }

            return result;
        }

        static std::string decode(const std::string& win1251_input)
        {
            std::string result;
            result.reserve(win1251_input.length() * 2);
            // UTF-8 might need more space

            for (const unsigned char c : win1251_input)
            {
                const uint32_t unicode = windows1251_to_unicode(c);
                append_utf8(result, unicode);
            }

            return result;
        }

    private:
        static size_t utf8_to_codepoint(const std::string& utf8_str,
            const size_t pos,
            uint32_t& codepoint)
        {
            unsigned char const first = utf8_str[pos];

            if ((first & 0x80) == 0)
            {
                codepoint = first;
                return 1;
            }
            if ((first & 0xE0) == 0xC0)
            {
                if (pos + 1 >= utf8_str.length()) return 0;
                codepoint = ((first & 0x1F) << 6) | (utf8_str[pos + 1] & 0x3F);
                return 2;
            }
            if ((first & 0xF0) == 0xE0)
            {
                if (pos + 2 >= utf8_str.length()) return 0;
                codepoint = ((first & 0x0F) << 12) | ((utf8_str[pos + 1] & 0x3F)
                    << 6) | (utf8_str[pos + 2] & 0x3F);
                return 3;
            }
            return 0;
        }

        static void append_utf8(std::string& str, const uint32_t codepoint)
        {
            if (codepoint <= 0x7F)
            {
                str.push_back(static_cast<char>(codepoint));
            }
            else if (codepoint <= 0x7FF)
            {
                str.push_back(
                    static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
                str.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
            else if (codepoint <= 0xFFFF)
            {
                str.push_back(
                    static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
                str.push_back(
                    static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                str.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
        }

        static uint32_t windows1251_to_unicode(const unsigned char win1251_char)
        {
            constexpr static std::array<uint32_t, 128> conversion_table = {
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
                0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F
            };

            if (win1251_char < 0x80)
            {
                return win1251_char;
            }
            return conversion_table[win1251_char - 0x80];
        }

        constexpr static uint8_t unicode_to_windows1251(const uint32_t unicode)
        {
            if (unicode < 0x80)
            {
                return static_cast<uint8_t>(unicode);
            }

            // Simple lookup table for common Cyrillic characters
            if (unicode >= 0x0410 && unicode <= 0x044F)
            {
                return static_cast<uint8_t>(unicode - 0x0350);
            }

            // Handle special cases
            switch (unicode)
            {
                case 0x2116: return 0xB9; // №
                case 0x0401: return 0xA8; // Ё
                case 0x0451: return 0xB8; // ё
                // Add more special cases as needed
                default: return '?';
                    // Return question mark for unsupported characters
            }
        }
    };
}

#endif //WINDOWS_1251_HPP
