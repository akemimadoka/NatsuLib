#pragma once
#include "natStream.h"
#include "natText.h"

#ifdef _MSC_VER
#	pragma push_macro("min")
#	undef min
#endif

namespace NatsuLib
{
	template <StringType encoding>
	class natStreamReader final
		: public natRefObjImpl<natStreamReader<encoding>, TextReader<encoding>>
	{
	public:
		typedef typename StringEncodingTrait<encoding>::CharType CharType;

		enum
		{
			DefaultBufferSize = 128,
		};

		explicit natStreamReader(natStream* pStream, size_t bufferSize = DefaultBufferSize) noexcept
			: m_InternalStream(pStream), m_BufferSize{ bufferSize }, m_CurrentPos{}, m_EndPos{}
		{
		}

		~natStreamReader()
		{
		}

		nBool Read(nuInt& codePoint) override
		{
			auto readChars = InternalPeek(codePoint);
			if (readChars)
			{
				m_CurrentPos += readChars * sizeof(CharType);
				assert(m_CurrentPos <= m_EndPos && "Buffer overflew.");
				return true;
			}

			return false;
		}

		nBool Peek(nuInt& codePoint) override
		{
			return InternalPeek(codePoint) > 0;
		}

		nBool IsEndOfStream() const
		{
			return m_InternalStream->IsEndOfStream();
		}

		natRefPointer<natStream> GetInternalStream() const noexcept
		{
			return m_InternalStream;
		}

	private:
		natRefPointer<natStream> m_InternalStream;
		std::vector<nByte> m_Buffer;
		size_t m_BufferSize;
		size_t m_CurrentPos, m_EndPos;

		// 清空缓冲区并重设位置，可选保留尾部的部分数据
		void ReadBuffer(size_t size, size_t reserved = 0)
		{
			if (!size)
			{
				return;
			}

			assert(size >= reserved);
			assert(reserved <= m_Buffer.size());
			copy(prev(cend(m_Buffer), reserved), cend(m_Buffer), begin(m_Buffer));
			m_Buffer.resize(size);
			
			const auto readBytes = m_InternalStream->ReadBytes(m_Buffer.data() + reserved, size - reserved);
			m_CurrentPos = 0;
			m_EndPos = static_cast<size_t>(reserved + readBytes);
		}

		size_t InternalPeek(nuInt& codePoint)
		{
			typedef typename StringEncodingTrait<encoding>::CharType CharType;
			assert(m_CurrentPos <= m_EndPos);
			if (m_CurrentPos == m_EndPos)
			{
				if (m_InternalStream->IsEndOfStream())
				{
					return 0;
				}

				ReadBuffer(m_BufferSize);
				if (m_CurrentPos == m_EndPos)
				{
					return 0;
				}
			}

			EncodingResult result;
			size_t readChars;
			const ncData data = m_Buffer.data();
			std::tie(result, readChars) = detail_::EncodingCodePoint<encoding>::Decode({ reinterpret_cast<const CharType*>(data + m_CurrentPos), reinterpret_cast<const CharType*>(data + std::min(m_EndPos, m_Buffer.size())) }, codePoint);
			if (result == EncodingResult::Accept)
			{
				return readChars;
			}
			if (result == EncodingResult::Incomplete)
			{
				ReadBuffer(m_BufferSize, m_EndPos - m_CurrentPos);
				std::tie(result, readChars) = detail_::EncodingCodePoint<encoding>::Decode({ reinterpret_cast<const CharType*>(data + m_CurrentPos), reinterpret_cast<const CharType*>(data + std::min(m_EndPos, m_Buffer.size())) }, codePoint);
				if (result == EncodingResult::Accept)
				{
					return readChars;
				}

				return 0;
			}

			return 0;
		}
	};

	template <StringType encoding>
	class natStreamWriter final
		: public natRefObjImpl<natStreamWriter<encoding>, TextWriter<encoding>>
	{
	public:
		explicit natStreamWriter(natStream* pStream) noexcept
			: m_InternalStream(pStream)
		{
		}

		~natStreamWriter()
		{
		}

		nBool Write(nuInt Char) override
		{
			String<encoding> tmpStr;
			if (detail_::EncodingCodePoint<encoding>::Encode(tmpStr, Char) == EncodingResult::Accept)
			{
				return Write(tmpStr) == tmpStr.GetView().GetCharCount();
			}
			return false;
		}

		size_t Write(StringView<encoding> const& str) override
		{
			typedef typename StringEncodingTrait<encoding>::CharType CharType;
			m_InternalStream->WriteBytes(reinterpret_cast<ncData>(str.cbegin()), str.size() * sizeof(CharType));
			return str.GetCharCount();
		}

		nBool IsEndOfStream() const
		{
			return m_InternalStream->IsEndOfStream();
		}

		natRefPointer<natStream> GetInternalStream() const noexcept
		{
			return m_InternalStream;
		}

	private:
		natRefPointer<natStream> m_InternalStream;
	};
}

#ifdef _MSC_VER
#	pragma pop_macro("min")
#endif
