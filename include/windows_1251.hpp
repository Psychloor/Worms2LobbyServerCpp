//
// Created by blomq on 2025-07-10.
//

#ifndef WINDOWS_1251_HPP
#define WINDOWS_1251_HPP

#include <string>

#include <icu.h>

namespace worms_server
{
	class windows_1251
	{
	public:
		static std::string encode(const std::string& utf8_input)
		{
			UErrorCode error = U_ZERO_ERROR;
			UConverter* converter = ucnv_open(windows1251_name(), &error);
			if (U_FAILURE(error))
			{
				return {};
			}

			std::string result(utf8_input.size() * 2, '\0'); // Buffer
			int32_t length = ucnv_convert(windows1251_name(), utf8_name(),
										  &result[0], static_cast<int32_t>(result.size()),
										  utf8_input.c_str(), static_cast<int32_t>(utf8_input.length()),
										  &error);

			ucnv_close(converter);
			if (U_FAILURE(error))
			{
				return {};
			}

			result.resize(length);
			return result;
		}

		static std::string decode(const std::string& win1251_input)
		{
			UErrorCode error = U_ZERO_ERROR;
			UConverter* converter = ucnv_open(windows1251_name(), &error);
			if (U_FAILURE(error))
			{
				return {};
			}

			std::string result(win1251_input.size() * 4, '\0'); // Buffer
			int32_t length = ucnv_convert(utf8_name(), windows1251_name(),
										  &result[0], static_cast<int32_t>(result.size()),
										  win1251_input.c_str(), static_cast<int32_t>(win1251_input.length()),
										  &error);

			ucnv_close(converter);
			if (U_FAILURE(error))
			{
				return {};
			}

			result.resize(length);
			return result;
		}

	private:
		static constexpr const char* windows1251_name()
		{
			return "windows-1251";
		}
		static constexpr const char* utf8_name()
		{
			return "UTF-8";
		}
	};
}

#endif //WINDOWS_1251_HPP
