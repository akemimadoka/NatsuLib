#pragma once
#include "natConfig.h"
#include "natEnvironment.h"
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
			m_NewLine.clear();
			EncodingResult result;
			nuInt codePoint;
			const auto charCount = newLine.GetCharCount();
			for (size_t i = 0; i < charCount; ++i)
			{
				std::tie(result, std::ignore) = detail_::EncodingCodePoint<encoding>::Decode(newLine.Slice(i, -1), codePoint);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "Not an available new line string."_nv);
				}

				m_NewLine.push_back(codePoint);
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
			nuInt currentChar;
			nuInt matchedLength{};
			String<encoding> result;
			while (matchedLength < m_NewLine.size() && Read(currentChar))
			{
				if (currentChar == m_NewLine[matchedLength])
				{
					++matchedLength;
				}
				else
				{
					matchedLength = 0;
				}

				if (detail_::EncodingCodePoint<encoding>::Encode(result, currentChar) != EncodingResult::Accept)
				{
					break;
				}
			}

			if (matchedLength == m_NewLine.size())
			{
				result.pop_back(matchedLength);
			}

			return result;
		}

		virtual String<encoding> ReadToEnd()
		{
			return ReadUntil({});
		}

	protected:
		TextReader()
		{
			TextReader::SetNewLine(String<encoding>{Environment::GetNewLine()});
		}

		std::vector<nuInt> m_NewLine;
	};

	template <StringType encoding>
	struct TextWriter
		: natRefObj
	{
		typedef typename StringEncodingTrait<encoding>::CharType CharType;

		virtual nBool Write(nuInt Char) = 0;

		virtual void SetNewLine(StringView<encoding> const& newLine)
		{
			m_NewLine.clear();
			EncodingResult result;
			nuInt codePoint;
			const auto charCount = newLine.GetCharCount();
			for (size_t i = 0; i < charCount; ++i)
			{
				std::tie(result, std::ignore) = detail_::EncodingCodePoint<encoding>::Decode(newLine.Slice(i, -1), codePoint);
				if (result != EncodingResult::Accept)
				{
					nat_Throw(natException, "Not an available new line string."_nv);
				}

				m_NewLine.push_back(codePoint);
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
			return size + WriteLine();
		}

		virtual size_t WriteLine()
		{
			for (auto&& item : m_NewLine)
			{
				Write(item);
			}

			return m_NewLine.size();
		}

	protected:
		TextWriter()
		{
			TextWriter::SetNewLine(String<encoding>{Environment::GetNewLine()});
		}

		std::vector<nuInt> m_NewLine;
	};
}
