#pragma once
#include "natException.h"
#include <cassert>
#include <sstream>
#include <tuple>
#include <iomanip>

namespace NatsuLib
{
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
					nat_Throw(natException, _T("Type {0} cannot convert to {1}"), C2Wstr(typeid(U).name()), C2Wstr(typeid(T).name()));
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

			constexpr static T&& Get(T&& value)
			{
				return value;
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

						nuInt tmpWidth = 0;
						while (_istdigit(*lpStr))
						{
							tmpWidth = tmpWidth * 10 + (*lpStr++ - _T('0'));
						}
						if (tmpWidth)
						{
							ss << std::setw(tmpWidth);
						}
						goto Begin;
					}
					break;
				case _T('{'):
				{
					nuInt tmpIndex = 0;
					++lpStr;
					while (*lpStr)
					{
						if (_istblank(*lpStr))
						{
							++lpStr;
							continue;
						}

						if (_istdigit(*lpStr))
						{
							tmpIndex = tmpIndex * 10 + (*lpStr++ - _T('0'));
							continue;
						}

						break;
					}

					while (_istblank(*lpStr)) { ++lpStr; }
					if (*lpStr != _T('}'))
					{
						nat_Throw(natException, _T("Expected '}', got '%c'"), *lpStr);
					}

					visit_at(argsTuple, tmpIndex, [&ss](auto&& item) { ss << item; });
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

		///	@brief	字符串分割函数
		///	@param[in]	str			要分割的字符串
		///	@param[in]	pattern		分割字符
		///	@param[in]	callableObject	可被调用的对象，接受参数为const Char_T*, size_t，分别为指向字符串开始位置的指针和字符串长度
		template <typename Char_T, typename CallableObject>
		void split(const Char_T* str, size_t strLen, const Char_T* pattern, size_t patternLen, CallableObject callableObject)
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
