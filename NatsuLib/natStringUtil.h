#pragma once
#include "natException.h"
#include <cassert>
#include <sstream>
#include <tuple>
#include <iomanip>
#include <locale>
#include <typeinfo>

namespace NatsuLib
{
	namespace natUtil
	{

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4800)
#endif

		namespace detail_
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
				[[noreturn]] static void visit(T&&, size_t, F)
				{
					nat_Throw(OutOfRange, "Out of range."_nv);
				}
			};

			template <bool test, bool test2, typename T>
			struct ExpectImpl
			{
				template <typename U>
				[[noreturn]] constexpr static T Get(U&&)
				{
					nat_Throw(natException, "Type {0} cannot be converted to {1}."_nv, typeid(U).name(), typeid(T).name());
				}
			};

			template <bool test2, typename T>
			struct ExpectImpl<true, test2, T>
			{
				template <typename U>
				constexpr static T Get(U&& value)
				{
					return static_cast<T>(std::forward<U>(value));
				}
			};

			template <typename T>
			struct ExpectImpl<false, true, T>
			{
				template <typename U>
				constexpr static T Get(U&& value)
				{
					return dynamic_cast<T>(std::forward<U>(value));
				}
			};
		}

		template <typename T>
		struct Expect
		{
			template <typename U>
			constexpr static T Get(U&& value)
			{
				return detail_::ExpectImpl<std::is_convertible<U, T>::value, false, T>::Get(std::forward<U>(value));
			}

			constexpr static decltype(auto) Get(T&& value)
			{
				return std::forward<T>(value);
			}
		};

		template <typename T>
		struct Expect<T*>
		{
			template <typename U>
			constexpr static T* Get(U&& value)
			{
				return detail_::ExpectImpl<std::is_convertible<U, T*>::value, false, T*>::Get(std::forward<U>(value));
			}

			template <typename U>
			constexpr static T* Get(U* value)
			{
				return detail_::ExpectImpl<std::is_convertible<U*, T*>::value, std::is_base_of<U, T>::value, T*>::Get(value);
			}

			constexpr static T* Get(T* value)
			{
				return value;
			}
		};

		template <typename T>
		struct Expect<T&>
		{
			template <typename U>
			constexpr static T& Get(U&& value)
			{
				return detail_::ExpectImpl<std::is_convertible<U, T&>::value, false, T&>::Get(std::forward<U>(value));
			}

			template <typename U>
			constexpr static T& Get(U& value)
			{
				return detail_::ExpectImpl<std::is_convertible<U&, T&>::value, std::is_base_of<U, T>::value, T&>::Get(value);
			}

			constexpr static T& Get(T& value)
			{
				return value;
			}
		};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

		template <typename F, template <typename...> class T, typename... Ts>
		void visit_at(T<Ts...> const& tup, size_t idx, F fun)
		{
			assert(idx < sizeof...(Ts));
			detail_::visit_impl<sizeof...(Ts)>::visit(tup, idx, fun);
		}

		template <typename F, template <typename...> class T, typename... Ts>
		void visit_at(T<Ts...>&& tup, size_t idx, F fun)
		{
			assert(idx < sizeof...(Ts));
			detail_::visit_impl<sizeof...(Ts)>::visit(tup, idx, fun);
		}

		template <typename... Args>
		nString FormatString(const nStrView::CharType* lpStr, Args&&... args)
		{
			std::basic_stringstream<nTChar> ss;
			nuInt index = 0;
			auto argsTuple = std::forward_as_tuple(args...);

			for (; *lpStr; ++lpStr)
			{
				switch (*lpStr)
				{
				case '%':
					++lpStr;
				Begin:
					switch (*lpStr)
					{
					case '%':
						ss << '%';
						break;
					case 'b':	// 扩展
						visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::boolalpha); ss << Expect<nBool>::Get(item); ss.setf(fmt); });
						break;
					case 'c':
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nTChar>::Get(item); });
						break;
					case 's':
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nString>::Get(item); });
						break;
					case 'd':
					case 'i':
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nInt>::Get(item); });
						break;
					case 'o':
						visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::oct, std::ios_base::basefield); ss << Expect<nuInt>::Get(item); ss.setf(fmt); });
						break;
					case 'x':
						visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss << Expect<nuInt>::Get(item); ss.setf(fmt); });
						break;
					case 'X':
						visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss.setf(std::ios_base::uppercase); ss << Expect<nuInt>::Get(item); ss.setf(fmt); });
						break;
					case 'u':
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nuInt>::Get(item); });
						break;
					case 'f':
					case 'F':
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nFloat>::Get(item); });
						break;
					case 'e':
						visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<nFloat>::Get(item); ss.setf(fmt); });
						break;
					case 'E':
						visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<nFloat>::Get(item); ss.setf(fmt); });
						break;
					case 'a':
						visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<nFloat>::Get(item); ss.setf(fmt); });
						break;
					case 'A':
						visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<nFloat>::Get(item); ss.setf(fmt); });
						break;
					case 'p':
						visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<const void*>::Get(item); });
						break;
					case 'h':
						switch (*++lpStr)
						{
						case 'd':
						case 'i':
							visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nShort>::Get(item); });
							break;
						case 'o':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::oct, std::ios_base::basefield); ss << Expect<nuShort>::Get(item); ss.setf(fmt); });
							break;
						case 'x':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss << Expect<nuShort>::Get(item); ss.setf(fmt); });
							break;
						case 'X':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss.setf(std::ios_base::uppercase); ss << Expect<nuShort>::Get(item); ss.setf(fmt); });
							break;
						case 'u':
							visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nuShort>::Get(item); });
							break;
						case 'h':
							switch (*++lpStr)
							{
							case 'd':
							case 'i':
								visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<int8_t>::Get(item); });
								break;
							case 'o':
								visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::oct, std::ios_base::basefield); ss << Expect<nByte>::Get(item); ss.setf(fmt); });
								break;
							case 'x':
								visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss << Expect<nByte>::Get(item); ss.setf(fmt); });
								break;
							case 'X':
								visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss.setf(std::ios_base::uppercase); ss << Expect<nByte>::Get(item); ss.setf(fmt); });
								break;
							case 'u':
								visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nByte>::Get(item); });
								break;
							default:
								nat_Throw(natException, "Unknown token '%c'"_nv, *lpStr);
							}
							break;
						default:
							nat_Throw(natException, "Unknown token '%c'"_nv, *lpStr);
						}
						break;
					case 'l':
						switch (*++lpStr)
						{
						case 'd':
						case 'i':
							visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<long>::Get(item); });
							break;
						case 'o':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::oct, std::ios_base::basefield); ss << Expect<unsigned long>::Get(item); ss.setf(fmt); });
							break;
						case 'x':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss << Expect<unsigned long>::Get(item); ss.setf(fmt); });
							break;
						case 'X':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss.setf(std::ios_base::uppercase); ss << Expect<unsigned long>::Get(item); ss.setf(fmt); });
							break;
						case 'u':
							visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<unsigned long>::Get(item); });
							break;
						case 'f':
						case 'F':
							visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nDouble>::Get(item); });
							break;
						case 'e':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<nDouble>::Get(item); ss.setf(fmt); });
							break;
						case 'E':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<nDouble>::Get(item); ss.setf(fmt); });
							break;
						case 'a':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<nDouble>::Get(item); ss.setf(fmt); });
							break;
						case 'A':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<nDouble>::Get(item); ss.setf(fmt); });
							break;
						case 'l':
							switch (*++lpStr)
							{
							case 'd':
							case 'i':
								visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nLong>::Get(item); });
								break;
							case 'o':
								visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::oct, std::ios_base::basefield); ss << Expect<nuLong>::Get(item); ss.setf(fmt); });
								break;
							case 'x':
								visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss << Expect<nuLong>::Get(item); ss.setf(fmt); });
								break;
							case 'X':
								visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::hex, std::ios_base::basefield); ss.setf(std::ios_base::uppercase); ss << Expect<nuLong>::Get(item); ss.setf(fmt); });
								break;
							case 'u':
								visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<nuLong>::Get(item); });
								break;
							default:
								break;
							}
							break;
						default:
							nat_Throw(natException, "Unknown token '%c'"_nv, *lpStr);
						}
						break;
					case 'L':
						switch (*++lpStr)
						{
						case 'f':
						case 'F':
							visit_at(argsTuple, index++, [&ss](auto&& item) { ss << Expect<long double>::Get(item); });
							break;
						case 'e':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<long double>::Get(item); ss.setf(fmt); });
							break;
						case 'E':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<long double>::Get(item); ss.setf(fmt); });
							break;
						case 'a':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss << Expect<long double>::Get(item); ss.setf(fmt); });
							break;
						case 'A':
							visit_at(argsTuple, index++, [&ss](auto&& item) { const auto fmt = ss.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield); ss.setf(std::ios_base::uppercase); ss << Expect<long double>::Get(item); ss.setf(fmt); });
							break;
						default:
							nat_Throw(natException, "Unknown token '%c'"_nv, *lpStr);
						}
						break;
					default:
						if (*lpStr == '0')
						{
							ss << std::setfill(*lpStr++);
						}

						if (!std::isdigit(*lpStr, std::locale{}))
						{
							nat_Throw(natException, "Unknown token '%c'"_nv, *lpStr);
						}

						nuInt tmpWidth = 0;
						while (std::isdigit(*lpStr, std::locale{}))
						{
							tmpWidth = tmpWidth * 10 + (*lpStr++ - '0');
						}
						if (tmpWidth)
						{
							ss << std::setw(tmpWidth);
						}
						goto Begin;
					}
					break;
				case '{':
				{
					nuInt tmpIndex = 0;
					++lpStr;
					while (*lpStr)
					{
						if (std::isblank(*lpStr, std::locale{}))
						{
							++lpStr;
							continue;
						}

						if (std::isdigit(*lpStr, std::locale{}))
						{
							tmpIndex = tmpIndex * 10 + (*lpStr++ - '0');
							continue;
						}

						break;
					}

					while (std::isblank(*lpStr, std::locale{})) { ++lpStr; }
					if (*lpStr != '}')
					{
						nat_Throw(natException, "Expected '}', got '%c'", *lpStr);
					}

					if (tmpIndex < std::tuple_size<decltype(argsTuple)>::value)
					{
						visit_at(argsTuple, tmpIndex, [&ss](auto&& item) { ss << item; });
					}
					
					break;
				}
				default:
					ss << *lpStr;
					break;
				}
			}

#ifdef _WIN32
			return { AnsiStringView{ ss.str().c_str() } };
#else
			return { U8StringView{ ss.str().c_str() } };
#endif
		}

		template <>
		inline nString FormatString(const nStrView::CharType* lpStr)
		{
			return { lpStr };
		}

		template <typename... Args>
		nString FormatString(nStrView const& Str, Args&&... args)
		{
			return FormatString(Str.data(), std::forward<Args>(args)...);
		}

		template <>
		inline nString FormatString(nStrView const& Str)
		{
			return Str;
		}
	}
}
