#pragma once
#include "natConfig.h"
#include "natString.h"
#include "natException.h"

namespace NatsuLib
{
	template <StringType stringType>
	struct RuntimeEncoding
	{
		static String<stringType> Encode(ncData data, size_t size, StringType encodingAs)
		{
			const auto encoding = encodingAs;

			switch (encoding)
			{
			case StringType::Utf8:
				return { U8StringView{ reinterpret_cast<const char*>(data), size } };
			case StringType::Utf16:
				return { U16StringView{ reinterpret_cast<const char16_t*>(data), size } };
			case StringType::Utf32:
				return { U32StringView{ reinterpret_cast<const char32_t*>(data), size} };
#ifdef _WIN32
			case StringType::Ansi:
				return { AnsiStringView{ reinterpret_cast<const char*>(data), size } };
			case StringType::Wide:
				return { WideStringView{ reinterpret_cast<const wchar_t*>(data), size } };
#endif
			default:
				nat_Throw(natErrException, NatErr_OutOfRange, "Wrong encodingAs."_nv);
			}
		}

		static void Decode(StringView<stringType> const& view, nData data, size_t size, StringType encodingAs)
		{
			const auto encoding = encodingAs;

			switch (encoding)
			{
			case StringType::Utf8:
			{
				String<StringType::Utf8> ret{ view };
				const auto retSize = ret.size() * sizeof(char);
				if (retSize > size)
				{
					nat_Throw(natErrException, NatErr_OutOfRange, "size is too small."_nv);
				}

#ifdef _MSC_VER
				memcpy_s(data, size, ret.data(), retSize);
#else
				memcpy(data, ret.data(), retSize);
#endif
			}
				break;
			case StringType::Utf16:
			{
				String<StringType::Utf16> ret{ view };
				const auto retSize = ret.size() * sizeof(char);
				if (retSize > size)
				{
					nat_Throw(natErrException, NatErr_OutOfRange, "size is too small."_nv);
				}

#ifdef _MSC_VER
				memcpy_s(data, size, ret.data(), retSize);
#else
				memcpy(data, ret.data(), retSize);
#endif
			}
				break;
			case StringType::Utf32:
			{
				String<StringType::Utf32> ret{ view };
				const auto retSize = ret.size() * sizeof(char);
				if (retSize > size)
				{
					nat_Throw(natErrException, NatErr_OutOfRange, "size is too small."_nv);
				}

#ifdef _MSC_VER
				memcpy_s(data, size, ret.data(), retSize);
#else
				memcpy(data, ret.data(), retSize);
#endif
			}
				break;
#ifdef _WIN32
			case StringType::Ansi:
			{
				String<StringType::Ansi> ret{ view };
				const auto retSize = ret.size() * sizeof(char);
				if (retSize > size)
				{
					nat_Throw(natErrException, NatErr_OutOfRange, "size is too small."_nv);
				}

#ifdef _MSC_VER
				memcpy_s(data, size, ret.data(), retSize);
#else
				memcpy(data, ret.data(), retSize);
#endif
			}
				break;
			case StringType::Wide:
			{
				String<StringType::Wide> ret{ view };
				const auto retSize = ret.size() * sizeof(wchar_t);
				if (retSize > size)
				{
					nat_Throw(natErrException, NatErr_OutOfRange, "size is too small."_nv);
				}

#ifdef _MSC_VER
				memcpy_s(data, size, ret.data(), retSize);
#else
				memcpy(data, ret.data(), retSize);
#endif
			}
				break;
#endif
			default:
				nat_Throw(natErrException, NatErr_OutOfRange, "Wrong encodingAs."_nv);
			}
		}
	};
}
