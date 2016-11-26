#pragma once
#include "natConfig.h"
#include "natString.h"
#include "natRefObj.h"

namespace NatsuLib
{
	template <StringType encoding>
	struct TextReader
		: natRefObj
	{
		typedef typename StringEncodingTrait<encoding>::CharType CharType;

		// 我们假设一个nuInt可以容纳任何NatsuLib支持的编码的单个有效字符
		// 注意：输出总是一个CodePoint，并非raw的字符组合
		virtual nBool Read(nuInt& Char) = 0;
		virtual nBool Peek(nuInt& Char) = 0;

		virtual void SetNewLine(StringView<encoding> const& newLine)
		{
			EncodingResult result;
			std::tie(result, std::ignore) = detail_::EncodingCodePoint<encoding>::Decode(newLine, m_NewLine);
			if (result != EncodingResult::Accept)
			{
				nat_Throw(natException, _T("Not an available new line string."));
			}
		}

		virtual String<encoding> ReadUntil(std::initializer_list<nuInt> terminatorChars)
		{
			nuInt currentChar;
			String<encoding> result;
			while (Read(currentChar)
				&& std::find(terminatorChars.begin(), terminatorChars.end(), currentChar) == terminatorChars.end()
				&& detail_::EncodingCodePoint<encoding>::Encode(result, currentChar) == EncodingResult::Accept)
			{
			}
			return result;
		}

		virtual String<encoding> ReadLine()
		{
			return ReadUntil({ m_NewLine });
		}

		virtual String<encoding> ReadToEnd()
		{
			return ReadUntil({});
		}

	protected:
		TextReader()
			: m_NewLine{ '\n' }
		{
		}

		// TODO: 修改为序列以支持"\r\n"这样的换行符
		nuInt m_NewLine;
	};

	template <StringType encoding>
	struct TextWriter
		: natRefObj
	{
		typedef typename StringEncodingTrait<encoding>::CharType CharType;

		virtual nBool Write(nuInt Char) = 0;

		virtual void SetNewLine(StringView<encoding> const& newLine)
		{
			EncodingResult result;
			std::tie(result, std::ignore) = detail_::EncodingCodePoint<encoding>::Decode(newLine, m_NewLine);
			if (result != EncodingResult::Accept)
			{
				nat_Throw(natException, _T("Not an available new line string."));
			}
		}

		virtual size_t Write(StringView<encoding> const& str)
		{
			size_t writtenChars{};
			nuInt codePoint;
			EncodingResult result;
			size_t moveStep;
			size_t currentPos{};

			while (true)
			{
				std::tie(result, moveStep) = detail_::EncodingCodePoint<encoding>::Decode(str.Slice(static_cast<std::ptrdiff_t>(currentPos), -1), codePoint);
				
				if (result != EncodingResult::Accept || !Write(codePoint))
				{
					break;
				}

				currentPos += moveStep;
				++writtenChars;
			}

			return writtenChars;
		}

		virtual size_t WriteLine(StringView<encoding> const& str)
		{
			const auto size = Write(str);
			return size + Write(m_NewLine) ? 1 : 0;
		}

	protected:
		TextWriter()
			: m_NewLine{ '\n' }
		{
		}

		nuInt m_NewLine;
	};
}
