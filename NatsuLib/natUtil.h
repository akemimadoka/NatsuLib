#pragma once
#include "natType.h"
#include <Windows.h>
#include <vector>
#include "natStringUtil.h"

namespace NatsuLib
{
	namespace natUtil
	{
		///	@brief	获得本地时间
		///	@return	包含时间信息的字符串
		inline nTString GetSysTime()
		{
			SYSTEMTIME st;
			GetLocalTime(&st);

			return FormatString(_T("%04d/%02d/%02d %02d:%02d:%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		}

		///	@brief	string转wstring
		inline std::wstring C2Wstr(std::string const& str)
		{
			return std::wstring(str.begin(), str.end());
		}
		///	@brief	wstring转string
		inline std::string W2Cstr(std::wstring const& str)
		{
			return std::string(str.begin(), str.end());
		}

		///	@brief	多字节转Unicode
		std::wstring MultibyteToUnicode(ncStr Str, nuInt CodePage = CP_INSTALLED);
		///	@brief	宽字符转多字节
		std::string WidecharToMultibyte(ncWStr Str, nuInt CodePage = CP_INSTALLED);

		///	@brief	获得资源字符串
		///	@param[in]	ResourceID	资源ID
		///	@param[in]	hInstance	实例句柄，默认为NULL
		nTString GetResourceString(DWORD ResourceID, HINSTANCE hInstance = NULL);

		///	@brief	获得资源数据
		///	@param[in]	ResourceID	资源ID
		///	@param[in]	lpType		资源类型
		///	@param[in]	hInstance	实例句柄，默认为NULL
		std::vector<nByte> GetResourceData(DWORD ResourceID, ncTStr lpType, HINSTANCE hInstance = NULL);

		///	@brief	字符串分割函数
		///	@param[in]	str			要分割的字符串
		///	@param[in]	pattern		分割字符
		///	@param[in]	callableObject	可被调用的对象，接受参数为nUnsafePtr<const Char_T>, size_t，分别为指向字符串开始位置的指针和字符串长度
		template <typename Char_T, typename CallableObject>
		void split(nUnsafePtr<const Char_T> str, size_t strLen, nUnsafePtr<const Char_T> pattern, size_t patternLen, CallableObject callableObject)
		{
			size_t pos = 0;

			for (size_t i = 0; i < strLen; ++i)
			{
				auto currentchar = str[i];
				for (size_t j = 0; j < patternLen; ++j)
				{
					if (str[i] == pattern[j])
					{
						callableObject(str + pos, i - pos);

						pos = i + 1;
						break;
					}
				}
			}

			if (pos != strLen)
			{
				callableObject(str + pos, strLen - pos);
			}
		}

		template <typename Char_T, typename CallableObject, size_t StrLen, size_t PatternLen>
		void split(const Char_T(&str)[StrLen], const Char_T(&pattern)[PatternLen], CallableObject callableObject)
		{
			split(str, StrLen - 1, pattern, PatternLen - 1, callableObject);
		}

		template <typename Char_T, typename CallableObject>
		void split(std::basic_string<Char_T> const& str, std::basic_string<Char_T> const& pattern, CallableObject callableObject)
		{
			split(str.c_str(), str.size(), pattern.c_str(), pattern.size(), callableObject);
		}
	}
}
