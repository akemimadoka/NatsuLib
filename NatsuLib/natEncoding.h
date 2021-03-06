#pragma once
#include "natConfig.h"
#include "natString.h"
#include "natException.h"

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	运行时编码
	///	@note	使得在运行时选择编码并进行编解码成为可能
	///	@tparam	stringType	要编码到/解码自的编码
	////////////////////////////////////////////////////////////////////////////////
	template <StringType stringType>
	struct RuntimeEncoding
	{
		static String<stringType> Encode(ncData data, size_t size, StringType encodingAs)
		{
			const auto encoding = encodingAs;

			switch (encoding)
			{
			case StringType::Utf8:
				return { U8StringView{ reinterpret_cast<const char*>(data), size / sizeof(char) } };
			case StringType::Utf16:
				return { U16StringView{ reinterpret_cast<const char16_t*>(data), size / sizeof(char16_t) } };
			case StringType::Utf32:
				return { U32StringView{ reinterpret_cast<const char32_t*>(data), size / sizeof(char32_t) } };
#ifdef _WIN32
			case StringType::Ansi:
				return { AnsiStringView{ reinterpret_cast<const char*>(data), size / sizeof(char) } };
			case StringType::Wide:
				return { WideStringView{ reinterpret_cast<const wchar_t*>(data), size / sizeof(wchar_t) } };
#endif
			default:
				assert(!"Invalid encodingAs.");
				nat_Throw(natErrException, NatErr_OutOfRange, "Invalid encodingAs."_nv);
			}
		}

		static std::vector<nByte> Decode(StringView<stringType> const& view, StringType encodingAs)
		{
			const auto encoding = encodingAs;

			switch (encoding)
			{
			case StringType::Utf8:
			{
				String<StringType::Utf8> ret{ view };
				return { reinterpret_cast<const nByte*>(ret.cbegin()), reinterpret_cast<const nByte*>(ret.cend()) };
			}
			case StringType::Utf16:
			{
				String<StringType::Utf16> ret{ view };
				return { reinterpret_cast<const nByte*>(ret.cbegin()), reinterpret_cast<const nByte*>(ret.cend()) };
			}
			case StringType::Utf32:
			{
				String<StringType::Utf32> ret{ view };
				return { reinterpret_cast<const nByte*>(ret.cbegin()), reinterpret_cast<const nByte*>(ret.cend()) };
			}
#ifdef _WIN32
			case StringType::Ansi:
			{
				String<StringType::Ansi> ret{ view };
				return { reinterpret_cast<const nByte*>(ret.cbegin()), reinterpret_cast<const nByte*>(ret.cend()) };
			}
			case StringType::Wide:
			{
				String<StringType::Wide> ret{ view };
				return { reinterpret_cast<const nByte*>(ret.cbegin()), reinterpret_cast<const nByte*>(ret.cend()) };
			}
#endif
			default:
				assert(!"Invalid encodingAs.");
				nat_Throw(natErrException, NatErr_OutOfRange, "Invalid encodingAs."_nv);
			}
		}
	};
}
