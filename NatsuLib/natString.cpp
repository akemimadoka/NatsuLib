#include "stdafx.h"
#include "natString.h"
#include "natUtil.h"

using namespace NatsuLib;

namespace NatsuLib
{
	template class String<StringType::Utf8>;
	template class String<StringType::Utf16>;
	template class String<StringType::Utf32>;

#ifdef _WIN32
	template class String<StringType::Ansi>;
	template class String<StringType::Wide>;
#endif
}

#define SET_REPLACEMENT_CHARACTER(codepoint) do { (codepoint) = 0xFFFD; } while (false)
#define DECODE_ERROR(resultCode, result, codepoint) { (result) = EncodingResult::resultCode; SET_REPLACEMENT_CHARACTER(codepoint); break; }

void detail_::IndexOutOfRange()
{
	nat_Throw(natException, "Index is out of range."_nv);
}

void detail_::SizeOutOfRange()
{
	nat_Throw(natException, "Size is out of range."_nv);
}

std::tuple<EncodingResult, char32_t, const char*> NatsuLib::DecodeUtf8(const char* strBegin, const char* strEnd) noexcept
{
	assert(strBegin && strEnd && "strBegin and strEnd should not be nullptr.");

	if (strBegin == strEnd)
	{
		return { EncodingResult::Incomplete, 0xFFFD, nullptr };
	}

	EncodingResult result = EncodingResult::Accept;
	auto current = strBegin;
	nuInt codePoint, test;

	const auto unit = static_cast<nByte>(*current++);

#define DECODE_ONE(codepoint, bits) \
{\
	const auto nextUnit = static_cast<nByte>(*current++);\
	nByte test_;\
	if ((test_ = nextUnit - 0x80) >= 0x40)\
	{\
		DECODE_ERROR(Reject, result, codepoint);\
	}\
	(codepoint) += test_ << (bits);\
}

	do
	{
		if (unit < 0x80)
		{
			codePoint = unit;
		}
		else if ((test = unit - 0x80) < 0x40)
		{
			DECODE_ERROR(Reject, result, codePoint);
		}
		else if ((test = unit - 0xC0) < 0x20)
		{
			if (strEnd - current < 1)
			{
				DECODE_ERROR(Incomplete, result, codePoint);
			}
			codePoint = test << 6;
			DECODE_ONE(codePoint, 0);
			if (codePoint < 0x80)
			{
				DECODE_ERROR(Reject, result, codePoint);
			}
		}
		else if ((test = unit - 0xE0) < 0x10)
		{
			if (strEnd - current < 2)
			{
				DECODE_ERROR(Incomplete, result, codePoint);
			}
			codePoint = test << 12;
			DECODE_ONE(codePoint, 6);
			DECODE_ONE(codePoint, 0);
			if (codePoint < 0x800 || codePoint - 0xD800 < 0x800)
			{
				DECODE_ERROR(Reject, result, codePoint);
			}
		}
		else if ((test = unit - 0xF0) < 0x08)
		{
			if (strEnd - current < 3)
			{
				DECODE_ERROR(Incomplete, result, codePoint);
			}
			codePoint = test << 18;
			DECODE_ONE(codePoint, 12);
			DECODE_ONE(codePoint, 6);
			DECODE_ONE(codePoint, 0);
			if (codePoint < 0x10000 || codePoint >= 0x110000)
			{
				DECODE_ERROR(Reject, result, codePoint);
			}
		}
		else
		{
			DECODE_ERROR(Reject, result, codePoint);
		}
	} while (false);

#undef DECODE_ONE

	assert(codePoint < 0x110000 && codePoint - 0xD800 >= 0x800 && "Incorrect codepoint.");
	return { result, static_cast<char32_t>(codePoint), current };
}

std::tuple<EncodingResult, char32_t, const char16_t*> NatsuLib::DecodeUtf16(const char16_t* strBegin, const char16_t* strEnd) noexcept
{
	assert(strBegin && strEnd && "strBegin and strEnd should not be nullptr.");

	if (strBegin == strEnd)
	{
		return { EncodingResult::Incomplete, 0xFFFD, nullptr };
	}

	EncodingResult result = EncodingResult::Accept;
	auto current = strBegin;
	nuInt codePoint, test;

	const auto unit = static_cast<nuShort>(*current++);

	do
	{
		if ((test = unit - 0xD800) < 0x400)
		{
			if (strEnd - current < 1)
			{
				DECODE_ERROR(Incomplete, result, codePoint);
			}
			codePoint = test << 10;
			const auto nextUnit = static_cast<nuShort>(*current++);
			if ((test = nextUnit - 0xDC00) >= 0x400)
			{
				DECODE_ERROR(Reject, result, codePoint);
			}
			codePoint += test;
			codePoint += 0x10000;
		}
		else if (test < 0x800)
		{
			DECODE_ERROR(Reject, result, codePoint);
		}
		else
		{
			codePoint = unit;
		}
	} while (false);
	
	assert(codePoint < 0x110000 && codePoint - 0xD800 >= 0x800 && "Incorrect codepoint.");
	return { result, static_cast<char32_t>(codePoint), current };
}

std::tuple<EncodingResult, char32_t, const char32_t*> NatsuLib::DecodeUtf32(const char32_t* strBegin, const char32_t* strEnd) noexcept
{
	assert(strBegin && strEnd && "strBegin and strEnd should not be nullptr.");

	if (strBegin == strEnd)
	{
		return { EncodingResult::Incomplete, 0xFFFD, nullptr };
	}

	EncodingResult result = EncodingResult::Accept;
	auto current = strBegin;
	nuInt codePoint;

	const auto unit = static_cast<nuInt>(*current++);

	do
	{
		if (unit - 0xD800 < 0x800)
		{
			DECODE_ERROR(Reject, result, codePoint);
		}

		if (unit < 0x110000)
		{
			codePoint = unit;
		}
		else
		{
			DECODE_ERROR(Reject, result, codePoint);
		}
	} while (false);
	
	assert(codePoint < 0x110000 && codePoint - 0xD800 >= 0x800 && "Incorrect codepoint.");
	return { result, static_cast<char32_t>(codePoint), current };
}

#define ENCODE_ERROR(resultCode, current) return { EncodingResult::resultCode, (current) }
#define ENCODE_CHECK_BUFFER(current, end, size) do { if ((end) - (current) < (size)) ENCODE_ERROR(Incomplete, current); } while (false)

std::pair<EncodingResult, char*> NatsuLib::EncodeUtf8(char* strBegin, const char* strEnd, char32_t input) noexcept
{
	assert(strBegin && strEnd && "strBegin and strEnd should not be nullptr.");

	const nuInt codePoint = input;
	auto current = strBegin;

	if (codePoint < 0x80)
	{
		ENCODE_CHECK_BUFFER(current, strEnd, 1);
		*current++ = static_cast<char>(codePoint);
	}
	else if (codePoint < 0x800)
	{
		ENCODE_CHECK_BUFFER(current, strEnd, 2);
		*current++ = static_cast<char>((codePoint >> 6) + 0xC0);
		*current++ = static_cast<char>(((codePoint << 26) >> 26) + 0x80);
	}
	else if (codePoint < 0x10000)
	{
		if (codePoint - 0xD800 < 0x800)
		{
			ENCODE_ERROR(Reject, current);
		}
		ENCODE_CHECK_BUFFER(current, strEnd, 3);
		*current++ = static_cast<char>((codePoint >> 12) + 0xE0);
		*current++ = static_cast<char>(((codePoint << 20) >> 26) + 0x80);
		*current++ = static_cast<char>(((codePoint << 26) >> 26) + 0x80);
	}
	else if (codePoint < 0x110000)
	{
		ENCODE_CHECK_BUFFER(current, strEnd, 4);
		*current++ = static_cast<char>((codePoint >> 18) + 0xF0);
		*current++ = static_cast<char>(((codePoint << 14) >> 26) + 0x80);
		*current++ = static_cast<char>(((codePoint << 20) >> 26) + 0x80);
		*current++ = static_cast<char>(((codePoint << 26) >> 26) + 0x80);
	}
	else
	{
		ENCODE_ERROR(Reject, current);
	}

	return { EncodingResult::Accept, current };
}

std::pair<EncodingResult, char16_t*> NatsuLib::EncodeUtf16(char16_t* strBegin, const char16_t* strEnd, char32_t input) noexcept
{
	assert(strBegin && strEnd && "strBegin and strEnd should not be nullptr.");

	const nuInt codePoint = input;
	auto current = strBegin;

	if (codePoint < 0x10000)
	{
		if (codePoint - 0xD800 < 0x800)
		{
			ENCODE_ERROR(Reject, current);
		}
		ENCODE_CHECK_BUFFER(current, strEnd, 1);
		*current++ = static_cast<char16_t>(codePoint);
	}
	else if (codePoint < 0x110000)
	{
		ENCODE_CHECK_BUFFER(current, strEnd, 2);
		const auto LeadingSurrogate = ((codePoint - 0x10000) >> 10) + 0xD800;
		const auto TrailingSurrogate = ((codePoint << 22) >> 22) + 0xDC00;
		*current++ = static_cast<char16_t>(LeadingSurrogate);
		*current++ = static_cast<char16_t>(TrailingSurrogate);
	}
	else
	{
		ENCODE_ERROR(Reject, current);
	}

	return { EncodingResult::Accept, current };
}

std::pair<EncodingResult, char32_t*> NatsuLib::EncodeUtf32(char32_t* strBegin, const char32_t* strEnd, char32_t input) noexcept
{
	assert(strBegin && strEnd && "strBegin and strEnd should not be nullptr.");

	const nuInt codePoint = input;
	auto current = strBegin;

	if (codePoint < 0x110000)
	{
		if (codePoint - 0xD800 < 0x800)
		{
			ENCODE_ERROR(Reject, current);
		}
		ENCODE_CHECK_BUFFER(current, strEnd, 1);
		*current++ = codePoint;
	}
	else
	{
		ENCODE_ERROR(Reject, current);
	}

	return { EncodingResult::Accept, current };
}

namespace NatsuLib
{
	namespace detail_
	{
		EncodingResult EncodingCodePoint<StringType::Utf8>::Encode(String<StringType::Utf8>& output, nuInt codePoint)
		{
			StringEncodingTrait<StringType::Utf8>::CharType buffer[StringEncodingTrait<StringType::Utf8>::MaxCharSize]{};
			EncodingResult result;
			const char* end;
			std::tie(result, end) = EncodeUtf8(std::begin(buffer), std::end(buffer), static_cast<char32_t>(codePoint));
			if (result == EncodingResult::Accept)
			{
				output.Append(StringView<StringType::Utf8>{ std::cbegin(buffer), end });
			}
			return result;
		}

		std::pair<EncodingResult, size_t> EncodingCodePoint<StringType::Utf8>::Decode(StringView<StringType::Utf8> const& input, nuInt& codePoint)
		{
			EncodingResult result;
			char32_t tmpCodePoint;
			const char* end;
			std::tie(result, tmpCodePoint, end) = DecodeUtf8(std::cbegin(input), std::cend(input));
			if (result == EncodingResult::Accept)
			{
				codePoint = static_cast<nuInt>(tmpCodePoint);
			}

			return { result, std::distance(std::cbegin(input), end) };
		}

		EncodingResult EncodingCodePoint<StringType::Utf16>::Encode(String<StringType::Utf16>& output, nuInt codePoint)
		{
			StringEncodingTrait<StringType::Utf16>::CharType buffer[StringEncodingTrait<StringType::Utf16>::MaxCharSize]{};
			EncodingResult result;
			const char16_t* end;
			std::tie(result, end) = EncodeUtf16(std::begin(buffer), std::end(buffer), static_cast<char32_t>(codePoint));
			if (result == EncodingResult::Accept)
			{
				output.Append(StringView<StringType::Utf16>{ std::cbegin(buffer), end });
			}
			return result;
		}

		std::pair<EncodingResult, size_t> EncodingCodePoint<StringType::Utf16>::Decode(StringView<StringType::Utf16> const& input, nuInt& codePoint)
		{
			EncodingResult result;
			char32_t tmpCodePoint;
			const char16_t* end;
			std::tie(result, tmpCodePoint, end) = DecodeUtf16(std::cbegin(input), std::cend(input));
			if (result == EncodingResult::Accept)
			{
				codePoint = static_cast<nuInt>(tmpCodePoint);
			}

			return{ result, std::distance(std::cbegin(input), end) };
		}

		EncodingResult EncodingCodePoint<StringType::Utf32>::Encode(String<StringType::Utf32>& output, nuInt codePoint)
		{
			StringEncodingTrait<StringType::Utf32>::CharType buffer[StringEncodingTrait<StringType::Utf32>::MaxCharSize]{};
			EncodingResult result;
			const char32_t* end;
			std::tie(result, end) = EncodeUtf32(std::begin(buffer), std::end(buffer), static_cast<char32_t>(codePoint));
			if (result == EncodingResult::Accept)
			{
				output.Append(StringView<StringType::Utf32>{ std::cbegin(buffer), end });
			}
			return result;
		}

		std::pair<EncodingResult, size_t> EncodingCodePoint<StringType::Utf32>::Decode(StringView<StringType::Utf32> const& input, nuInt& codePoint)
		{
			EncodingResult result;
			char32_t tmpCodePoint;
			const char32_t* end;
			std::tie(result, tmpCodePoint, end) = DecodeUtf32(std::cbegin(input), std::cend(input));
			if (result == EncodingResult::Accept)
			{
				codePoint = static_cast<nuInt>(tmpCodePoint);
			}

			return{ result, std::distance(std::cbegin(input), end) };
		}

#ifdef _WIN32
		EncodingResult EncodingCodePoint<StringType::Ansi>::Encode(String<StringType::Ansi>& output, nuInt codePoint)
		{
			try
			{
				output.Append(StringView<StringType::Utf32>{ codePoint });
			}
			catch (...)
			{
				return EncodingResult::Reject;	// 异常被吞
			}

			return EncodingResult::Accept;
		}

		std::pair<EncodingResult, size_t> EncodingCodePoint<StringType::Ansi>::Decode(StringView<StringType::Ansi> const& input, nuInt& codePoint)
		{
			codePoint = static_cast<nuInt>(*input.begin());	// 我们无法假设Ansi的字符串的一个有效字符占用的字节数
			return { EncodingResult::Accept, 1 };
		}

		EncodingResult EncodingCodePoint<StringType::Wide>::Encode(String<StringType::Wide>& output, nuInt codePoint)
		{
			// 假设 Windows 平台下的宽字符字符串总为UTF-16
			return EncodingCodePoint<StringType::Utf16>::Encode(reinterpret_cast<String<StringType::Utf16>&>(output), codePoint);
		}

		std::pair<EncodingResult, size_t> EncodingCodePoint<StringType::Wide>::Decode(StringView<StringType::Wide> const& input, nuInt& codePoint)
		{
			return EncodingCodePoint<StringType::Utf16>::Decode(reinterpret_cast<StringView<StringType::Utf16> const&>(input), codePoint);
		}

#endif
	}

	template <>
	void U8String::TransAppendTo(U16String& dst, View const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size());

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf8(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf8 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf16(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf16 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U8String::TransAppendFrom(U8String& dst, U16StringView const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size() * 3);

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf16(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf16 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf8(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf8 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U8String::TransAppendTo(U32String& dst, View const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size());

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf8(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf8 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf32(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf32 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U8String::TransAppendFrom(U8String& dst, U32StringView const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size() * 4);

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf32(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf32 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf8(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf8 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U16String::TransAppendTo(U16String& dst, View const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size());

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf16(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf16 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf16(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf16 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U16String::TransAppendFrom(U16String& dst, U16StringView const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size());

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf16(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf16 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf16(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf16 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U16String::TransAppendTo(U32String& dst, View const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size());

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf16(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf16 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf32(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf32 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U16String::TransAppendFrom(U16String& dst, U32StringView const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size() * 2);

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf32(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf32 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf16(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf16 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U32String::TransAppendTo(U16String& dst, View const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size() * 2);

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf32(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf32 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf16(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf16 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U32String::TransAppendFrom(U32String& dst, U16StringView const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size());

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf16(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf16 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf32(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf32 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U32String::TransAppendTo(U32String& dst, View const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size());

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf32(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf32 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf32(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf32 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

	template <>
	void U32String::TransAppendFrom(U32String& dst, U32StringView const& src)
	{
		const auto dstBegin = dst.ResizeMore(src.size());

		try
		{
			auto write = dstBegin;
			const auto writeEnd = dst.end();
			auto read = src.cbegin();
			const auto readEnd = src.cend();

			EncodingResult result;
			char32_t u32CodePoint;

			while (read < readEnd)
			{
				std::tie(result, u32CodePoint, read) = DecodeUtf32(read, readEnd);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "DecodeUtf32 failed."_nv);
				}
				std::tie(result, write) = EncodeUtf32(write, writeEnd, u32CodePoint);
				assert(result == EncodingResult::Accept && "EncodeUtf32 failed.");
			}
			dst.Resize(static_cast<size_t>(write - dst.begin()));
		}
		catch (...)
		{
			dst.Resize(static_cast<size_t>(dstBegin - dst.begin()));
			throw;
		}
	}

#ifdef _WIN32
	template <>
	void AnsiString::TransAppendTo(U16String& dst, View const& src)
	{
		const auto convertedString = natUtil::MultibyteToUnicode(src.cbegin(), static_cast<nInt>(src.size()), CP_ACP);
		const auto iter = dst.ResizeMore(convertedString.size());
		std::memmove(iter, convertedString.c_str(), convertedString.size() * sizeof(wchar_t));
	}

	template <>
	void AnsiString::TransAppendFrom(AnsiString& dst, U16StringView const& src)
	{
		const auto convertedString = natUtil::WidecharToMultibyte(reinterpret_cast<ncWStr>(src.data()), static_cast<nInt>(src.size()), CP_ACP);
		const auto iter = dst.ResizeMore(convertedString.size());
		std::memmove(iter, convertedString.c_str(), convertedString.size() * sizeof(char));
	}

	template <>
	void AnsiString::TransAppendTo(U32String& dst, View const& src)
	{
		U16String tmpStr;
		TransAppendTo(tmpStr, src);
		U16String::TransAppendTo(dst, tmpStr);
	}

	template <>
	void AnsiString::TransAppendFrom(AnsiString& dst, U32StringView const& src)
	{
		U16String tmpStr;
		U16String::TransAppendFrom(tmpStr, src);
		TransAppendFrom(dst, tmpStr);
	}

	template <>
	void WideString::TransAppendTo(U16String& dst, View const& src)
	{
		U16String::TransAppendTo(dst, reinterpret_cast<U16StringView const&>(src));
	}

	template <>
	void WideString::TransAppendFrom(WideString& dst, U16StringView const& src)
	{
		U16String::TransAppendFrom(reinterpret_cast<U16String&>(dst), src);
	}

	template <>
	void WideString::TransAppendTo(U32String& dst, View const& src)
	{
		U16String::TransAppendTo(dst, reinterpret_cast<U16StringView const&>(src));
	}

	template <>
	void WideString::TransAppendFrom(WideString& dst, U32StringView const& src)
	{
		U16String::TransAppendFrom(reinterpret_cast<U16String&>(dst), src);
	}
#endif
}
