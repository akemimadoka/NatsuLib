#pragma once
#include "natType.h"
#include <Windows.h>
#include <vector>
#include <cassert>
#include <tuple>
#include <sstream>
#include <iomanip>
#include "natException.h"

namespace natUtil
{
	namespace _Detail
	{
		template <size_t I>
		struct visit_impl
		{
			template <typename T, typename F>
			static void visit(T&& tup, size_t idx, F fun)
			{
				if (idx == I - 1) fun(std::get<I - 1>(tup));
				else visit_impl<I - 1>::visit(tup, idx, fun);
			}
		};

		template <>
		struct visit_impl<0>
		{
			template <typename T, typename F>
			static void visit(T&&, size_t, F) { assert(false); }
		};

		template <bool test, typename T>
		struct ExpectImpl
		{
			template <typename U>
			constexpr static T Get(U&&)
			{
				nat_Throw(natException, _T("Test failed"));
			}
		};

		template <typename T>
		struct ExpectImpl<true, T>
		{
			template <typename U>
			constexpr static T Get(U&& value)
			{
				return static_cast<T>(value);
			}
		};
	}

	template <typename T>
	struct Expect
	{
		template <typename U>
		constexpr static T Get(U&& value)
		{
			return _Detail::ExpectImpl<std::is_convertible<U, T>::value, T>::Get(value);
		}
	};

	template <typename F, template <typename...> class T, typename... Ts>
	void visit_at(T<Ts...> const& tup, size_t idx, F fun)
	{
		assert(idx < sizeof...(Ts));
		_Detail::visit_impl<sizeof...(Ts)>::visit(tup, idx, fun);
	}

	template <typename F, template <typename...> class T, typename... Ts>
	void visit_at(T<Ts...>&& tup, size_t idx, F fun)
	{
		assert(idx < sizeof...(Ts));
		_Detail::visit_impl<sizeof...(Ts)>::visit(tup, idx, fun);
	}

	template <typename... Args>
	nTString FormatString(ncTStr lpStr, Args&&... args)
	{
		std::basic_stringstream<nTChar> ss;
		nuInt index = 0;
		auto argsTuple = std::forward_as_tuple(args...);

		for (; *lpStr; ++lpStr)
		{
			switch (*lpStr)
			{
			case _T('%'):
				++lpStr;
			Begin:
				switch (*lpStr)
				{
				case _T('%'):
					ss << _T('%');
					break;
				case _T('c'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nTChar>::Get(item); });
					break;
				case _T('s'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nTString>::Get(item); });
					break;
				case _T('d'):
				case _T('i'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nInt>::Get(item); });
					break;
				case _T('o'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::oct, std::ios_base::basefield); ss << Expect<nuInt>::Get(item); ss.setf(fmt); });
					break;
				case _T('x'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss << Expect<nuInt>::Get(item); ss.setf(fmt); });
					break;
				case _T('X'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss.setf(std::ios_base::uppercase); ss << Expect<nuInt>::Get(item); ss.setf(fmt); });
					break;
				case _T('u'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nuInt>::Get(item); });
					break;
				case _T('f'):
				case _T('F'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nFloat>::Get(item); });
					break;
				case _T('e'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<nFloat>::Get(item); ss.setf(fmt); });
					break;
				case _T('E'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<nFloat>::Get(item); ss.setf(fmt); });
					break;
				case _T('a'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<nFloat>::Get(item); ss.setf(fmt); });
					break;
				case _T('A'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<nFloat>::Get(item); ss.setf(fmt); });
					break;
				case _T('p'):
					visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<const void*>::Get(item); });
					break;
				case _T('h'):
					switch (*++lpStr)
					{
					case _T('d'):
					case _T('i'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nShort>::Get(item); });
						break;
					case _T('o'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::oct, std::ios_base::basefield); ss << Expect<nuShort>::Get(item); ss.setf(fmt); });
						break;
					case _T('x'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss << Expect<nuShort>::Get(item); ss.setf(fmt); });
						break;
					case _T('X'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss.setf(std::ios_base::uppercase); ss << Expect<nuShort>::Get(item); ss.setf(fmt); });
						break;
					case _T('u'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nuShort>::Get(item); });
						break;
					case _T('h'):
						switch (*++lpStr)
						{
						case _T('d'):
						case _T('i'):
							visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<int8_t>::Get(item); });
							break;
						case _T('o'):
							visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::oct, std::ios_base::basefield); ss << Expect<nByte>::Get(item); ss.setf(fmt); });
							break;
						case _T('x'):
							visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss << Expect<nByte>::Get(item); ss.setf(fmt); });
							break;
						case _T('X'):
							visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss.setf(std::ios_base::uppercase); ss << Expect<nByte>::Get(item); ss.setf(fmt); });
							break;
						case _T('u'):
							visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nByte>::Get(item); });
							break;
						default:
							nat_Throw(natException, _T("Unknown token '%c'"), *lpStr);
						}
						break;
					default:
						nat_Throw(natException, _T("Unknown token '%c'"), *lpStr);
					}
					break;
				case _T('l'):
					switch (*++lpStr)
					{
					case _T('d'):
					case _T('i'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<long>::Get(item); });
						break;
					case _T('o'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::oct, std::ios_base::basefield); ss << Expect<unsigned long>::Get(item); ss.setf(fmt); });
						break;
					case _T('x'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss << Expect<unsigned long>::Get(item); ss.setf(fmt); });
						break;
					case _T('X'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss.setf(std::ios_base::uppercase); ss << Expect<unsigned long>::Get(item); ss.setf(fmt); });
						break;
					case _T('u'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<unsigned long>::Get(item); });
						break;
					case _T('f'):
					case _T('F'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nDouble>::Get(item); });
						break;
					case _T('e'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<nDouble>::Get(item); ss.setf(fmt); });
						break;
					case _T('E'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<nDouble>::Get(item); ss.setf(fmt); });
						break;
					case _T('a'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<nDouble>::Get(item); ss.setf(fmt); });
						break;
					case _T('A'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<nDouble>::Get(item); ss.setf(fmt); });
						break;
					case _T('l'):
						switch (*++lpStr)
						{
						case _T('d'):
						case _T('i'):
							visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nLong>::Get(item); });
							break;
						case _T('o'):
							visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::oct, std::ios_base::basefield); ss << Expect<nuLong>::Get(item); ss.setf(fmt); });
							break;
						case _T('x'):
							visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss << Expect<nuLong>::Get(item); ss.setf(fmt); });
							break;
						case _T('X'):
							visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss.setf(std::ios_base::uppercase); ss << Expect<nuLong>::Get(item); ss.setf(fmt); });
							break;
						case _T('u'):
							visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nuLong>::Get(item); });
							break;
						default:
							break;
						}
						break;
					default:
						nat_Throw(natException, _T("Unknown token '%c'"), *lpStr);
					}
					break;
				case _T('L'):
					switch (*++lpStr)
					{
					case _T('f'):
					case _T('F'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<long double>::Get(item); });
						break;
					case _T('e'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<long double>::Get(item); ss.setf(fmt); });
						break;
					case _T('E'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<long double>::Get(item); ss.setf(fmt); });
						break;
					case _T('a'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<long double>::Get(item); ss.setf(fmt); });
						break;
					case _T('A'):
						visit_at(argsTuple, index++, [&ss](auto&& item) { auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<long double>::Get(item); ss.setf(fmt); });
						break;
					default:
						nat_Throw(natException, _T("Unknown token '%c'"), *lpStr);
					}
					break;
				default:
					if (!_istdigit(*lpStr))
					{
						nat_Throw(natException, _T("Unknown token '%c'"), *lpStr);
					}
					else if (*lpStr == _T('0'))
					{
						ss << std::setfill(*lpStr++);
					}
					
					nTString tmpWidth;
					while (_istdigit(*lpStr))
					{
						tmpWidth += *lpStr++;
					}
					if (!tmpWidth.empty())
					{
						ss << std::setw(_ttoi(tmpWidth.c_str()));
					}
					goto Begin;
				}
				break;
			case _T('{'):
			{
				nTString tmpIndex;
				++lpStr;
				while (_istdigit(*lpStr))
				{
					tmpIndex += *lpStr++;
				}

				if (*lpStr != _T('}'))
				{
					nat_Throw(natException, _T("Expected '}', got '%c'"), *lpStr);
				}

				visit_at(argsTuple, _ttoi(tmpIndex.data()), [&ss](auto&& item) { ss << item; });
				break;
			}
			default:
				ss << *lpStr;
				break;
			}
		}

		return ss.str();
	}

	template <>
	inline nTString FormatString(ncTStr lpStr) noexcept
	{
		return lpStr;
	}

	template <typename... Args>
	nTString FormatString(nTString const& Str, Args&&... args)
	{
		return FormatString(Str.c_str(), std::forward<Args>(args)...);
	}

	template <>
	inline nTString FormatString(nTString const& Str)
	{
		return Str;
	}

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
	///	@param[out]	container	存储结果的容器，应实现emplace_back，接受参数为 字符串, 字符起始位置, 字符串长度
	template <typename Char_T, typename Container>
	void split(std::basic_string<Char_T> const& str, std::basic_string<Char_T> const& pattern, Container& container)
	{
		typedef typename std::basic_string<Char_T>::size_type pos_t;
		const auto size = str.size();

		pos_t pos = 0;

		for (pos_t i = 0u; i < size; ++i)
		{
			auto currentchar = str[i];
			for (auto c : pattern)
			{
				if (currentchar == c)
				{
					container.emplace_back(str, pos, i - pos);

					pos = i + 1;
					break;
				}
			}
		}

		if (pos != size)
		{
			container.emplace_back(str, pos, size - pos);
		}
	}
}
