#include "stdafx.h"
#include "natCryptography.h"
#include "gzguts.h"
#include <random>

using namespace NatsuLib;

namespace NatsuLib
{
	namespace detail_
	{
		constexpr NATINLINE nuInt Crc32One(const nuInt* crc32Table, nuInt c, nuInt b) noexcept
		{
			return ((crc32Table[((c ^ b) & 0xff)]) ^ (c >> 8));
		}

		constexpr NATINLINE nuInt DecryptByte(const nuInt* pKeys) noexcept
		{
			const auto temp = (pKeys[2] & 0xffff) | 2;
			return ((temp * (temp ^ 1)) >> 8) & 0xff;
		}

		constexpr NATINLINE void UpdateKeys(nuInt* pKeys, const nuInt* crc32Table, nuInt c) noexcept
		{
			pKeys[0] = Crc32One(crc32Table, pKeys[0], c);
			pKeys[1] += pKeys[0] & 0xff;
			pKeys[1] = pKeys[1] * 134775813u + 1;

			pKeys[2] = Crc32One(crc32Table, pKeys[2], pKeys[1] >> 24);
		}

		constexpr NATINLINE void InitKeys(ncData password, size_t passwordLength, nuInt* pKeys, const nuInt* crc32Table) noexcept
		{
			pKeys[0] = 305419896u;
			pKeys[1] = 591751049u;
			pKeys[2] = 878082192u;

			const auto pRead = password;
			const auto pEnd = password + passwordLength;

			for (const auto item : make_range(pRead, pEnd))
			{
				UpdateKeys(pKeys, crc32Table, item);
			}
		}

		constexpr NATINLINE nuInt DecodeOne(nuInt* pKeys, const nuInt* crc32Table, nuInt c) noexcept
		{
			UpdateKeys(pKeys, crc32Table, c ^= DecryptByte(pKeys));
			return c;
		}

		constexpr NATINLINE nuInt EncodeOne(nuInt* pKeys, const nuInt* crc32Table, nuInt c) noexcept
		{
			const auto t = DecryptByte(pKeys) ^ c;
			UpdateKeys(pKeys, crc32Table, c);
			return t;
		}

		// 不检查缓存大小，直接生成，调用者需保证调用时buf的大小不小于PKzipWeakProcessor::HeaderSize
		void UncheckedCryptHead(nData buf, nuInt* pKeys, const nuInt* crc32Table, nuInt crcForCrypting) noexcept
		{
			std::random_device randomDevice;
			std::generate_n(buf, PKzipWeakProcessor::HeaderSize - 2, [&]()
			{
				return static_cast<nByte>(EncodeOne(pKeys, crc32Table, static_cast<nuInt>(randomDevice() & 0xff)));
			});

			buf[PKzipWeakProcessor::HeaderSize - 2] = static_cast<nByte>(EncodeOne(pKeys, crc32Table, (crcForCrypting >> 16) & 0xff));
			buf[PKzipWeakProcessor::HeaderSize - 1] = static_cast<nByte>(EncodeOne(pKeys, crc32Table, (crcForCrypting >> 24) & 0xff));
		}

		nBool CryptHead(ncData password, size_t passwordLength, nData buf, size_t bufSize, nuInt* pKeys, const nuInt* crc32Table, nuInt crcForCrypting) noexcept
		{
			if (bufSize < PKzipWeakProcessor::HeaderSize)
			{
				return false;
			}

			InitKeys(password, passwordLength, pKeys, crc32Table);
			UncheckedCryptHead(buf, pKeys, crc32Table, crcForCrypting);

			return true;
		}
	}
}

ICryptoProcessor::~ICryptoProcessor()
{
}

ICryptoAlgorithm::~ICryptoAlgorithm()
{
}

PKzipWeakProcessor::PKzipWeakProcessor(CryptoType cryptoType)
	: m_IsCrypt{ cryptoType == CryptoType::Crypt }
{
}

PKzipWeakProcessor::~PKzipWeakProcessor()
{
}

void PKzipWeakProcessor::InitCipher(ncData password, nLen passwordLength)
{
	m_Keys.emplace();
	detail_::InitKeys(password, passwordLength, m_Keys.value().data(), get_crc_table());
}

void PKzipWeakProcessor::InitHeaderFrom(ncData buffer, nLen bufferLength)
{
	assert(!m_IsCrypt && "No need to initialize header in crypt mode.");

	if (!m_Keys)
	{
		nat_Throw(natErrException, NatErr_InternalErr, "Keys not initialized."_nv);
	}

	if (!m_Header)
	{
		m_Header.emplace();
	}

	auto& header = m_Header.value();
	std::memmove(header.data(), buffer, bufferLength);
	decryptHeader();
}

void PKzipWeakProcessor::InitHeaderFrom(natRefPointer<natStream> const& stream)
{
	assert(!m_IsCrypt && "No need to initialize header in crypt mode.");

	if (!m_Keys)
	{
		nat_Throw(natErrException, NatErr_InternalErr, "Keys not initialized."_nv);
	}

	if (!m_Header)
	{
		m_Header.emplace();
	}

	auto& header = m_Header.value();
	stream->ReadBytes(header.data(), sizeof header);
	decryptHeader();
}

nBool PKzipWeakProcessor::GetHeader(nData buffer, nLen bufferLength)
{
	if (!m_IsCrypt || !m_Header || bufferLength < HeaderSize)
	{
		return false;
	}

	std::memmove(buffer, m_Header.value().data(), HeaderSize);
	m_Header.reset();

	return true;
}

nBool PKzipWeakProcessor::CheckHeaderWithCrc32(nuInt crc32) const
{
	if (m_IsCrypt)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Processor is not in decrypt mode."_nv);
	}
	if (!m_Header)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Header not prepared."_nv);
	}
	if (!m_Keys)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Keys not prepared, header may not be decrypted."_nv);
	}

	const auto& header = m_Header.value();
	return header[HeaderSize - 2] == static_cast<nByte>((crc32 >> 16) & 0xff) && header[HeaderSize - 1] == static_cast<nByte>((crc32 >> 24) & 0xff);
}

void PKzipWeakProcessor::GenerateHeaderWithCrc32(nuInt crc32)
{
	if (!m_IsCrypt)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Processor is not in crypt mode."_nv);
	}
	if (!m_Keys)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Keys not prepared."_nv);
	}

	const auto crc32Table = get_crc_table();
	m_Header.emplace();
	auto& header = m_Header.value();
	auto& keys = m_Keys.value();

	detail_::UncheckedCryptHead(header.data(), keys.data(), crc32Table, crc32);
}

CryptoType PKzipWeakProcessor::GetCryptoType() const noexcept
{
	return m_IsCrypt ? CryptoType::Crypt : CryptoType::Decrypt;
}

nLen PKzipWeakProcessor::GetInputBlockSize() const noexcept
{
	return InputBlockSize;
}

nLen PKzipWeakProcessor::GetOutputBlockSize() const noexcept
{
	return OutputBlockSize;
}

nBool PKzipWeakProcessor::CanProcessMultiBlocks() const noexcept
{
	return true;
}

nBool PKzipWeakProcessor::CanReuseProcessor() const noexcept
{
	return true;
}

std::pair<nLen, nLen> PKzipWeakProcessor::Process(ncData inputData, nLen inputDataLength, nData outputData, nLen outputDataLength)
{
	if (!m_Keys)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Processor has not initialized. (Have you called InitCipher first?)"_nv);
	}

	if (outputDataLength < inputDataLength)
	{
		nat_Throw(OutOfRange, "outputData is too small."_nv);
	}

	uncheckedProcess(inputData, inputDataLength, outputData);
	return { inputDataLength, inputDataLength };
}

std::pair<nLen, std::vector<nByte>> PKzipWeakProcessor::ProcessFinal(ncData inputData, nLen inputDataLength)
{
	std::vector<nByte> outputBuffer(inputDataLength);
	const auto ret = Process(inputData, inputDataLength, outputBuffer.data(), outputBuffer.size());
	m_Keys.reset();
	return { ret.first, move(outputBuffer) };
}

void PKzipWeakProcessor::uncheckedProcess(ncData inputData, nLen inputDataLength, nData outputData)
{
	assert(m_Keys);
	auto keys = m_Keys.value().data();

	for (nLen i = 0; i < inputDataLength; ++i)
	{
		outputData[i] = static_cast<nByte>((m_IsCrypt ? detail_::EncodeOne : detail_::DecodeOne)(keys, get_crc_table(), inputData[i]));
	}
}

void PKzipWeakProcessor::decryptHeader()
{
	assert(m_Header);
	const auto header = m_Header.value().data();
	uncheckedProcess(header, HeaderSize, header);
}

natRefPointer<ICryptoProcessor> PKzipWeakAlgorithm::CreateEncryptor()
{
	return make_ref<PKzipWeakProcessor>(CryptoType::Crypt);
}

natRefPointer<ICryptoProcessor> PKzipWeakAlgorithm::CreateDecryptor()
{
	return make_ref<PKzipWeakProcessor>(CryptoType::Decrypt);
}

natCryptoStream::natCryptoStream(natRefPointer<natStream> stream, natRefPointer<ICryptoProcessor> cryptoProcessor, CryptoStreamMode mode)
	: natRefObjImpl{ std::move(stream) },
	  m_Processor{ std::move(cryptoProcessor) },
	  m_InputBlockSize{ m_Processor->GetInputBlockSize() }, m_OutputBlockSize{ m_Processor->GetOutputBlockSize() },
	  m_IsReadMode{ mode == CryptoStreamMode::Read },
	  m_InputBufferSize{}, m_OutputBufferSize{},
	  m_InputBuffer(m_InputBlockSize), m_OutputBuffer(m_OutputBlockSize),
	  m_FinalBlockProcessed{ false }
{
	if (!m_InternalStream)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "stream is nullptr."_nv);
	}
	if (!m_Processor)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "cryptoProcessor is nullptr."_nv);
	}
	if (m_IsReadMode && !m_InternalStream->CanRead())
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "stream should be readable in read mode."_nv);
	}
	if (!m_IsReadMode && !m_InternalStream->CanWrite())
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "stream should be writable in write mode."_nv);
	}
}

natCryptoStream::~natCryptoStream()
{
	if (!m_FinalBlockProcessed)
	{
		FlushFinalBlock();
	}
}

natRefPointer<ICryptoProcessor> natCryptoStream::GetProcessor() const noexcept
{
	return m_Processor;
}

nBool natCryptoStream::CanWrite() const
{
	return !m_IsReadMode;
}

nBool natCryptoStream::CanRead() const
{
	return m_IsReadMode;
}

nBool natCryptoStream::CanResize() const
{
	return false;
}

nBool natCryptoStream::CanSeek() const
{
	return false;
}

nLen natCryptoStream::GetSize() const
{
	nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
}

void natCryptoStream::SetSize(nLen Size)
{
	nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
}

nLen natCryptoStream::GetPosition() const
{
	nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
}

void natCryptoStream::SetPosition(NatSeek, nLong)
{
	nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
}

nLen natCryptoStream::ReadBytes(nData pData, nLen Length)
{
	if (!m_IsReadMode)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "This stream cannot be read."_nv);
	}

	auto bytesToRead = Length;
	auto pWrite = pData;

	if (m_OutputBufferSize)
	{
		if (m_OutputBufferSize < bytesToRead)
		{
			std::memmove(pWrite, m_OutputBuffer.data(), m_OutputBufferSize);
			bytesToRead -= m_OutputBufferSize;
			pWrite += m_OutputBufferSize;
			m_OutputBufferSize = 0;
		}
		else
		{
			std::memmove(pWrite, m_OutputBuffer.data(), bytesToRead);
			std::memmove(m_OutputBuffer.data(), m_OutputBuffer.data() + bytesToRead, m_OutputBufferSize - bytesToRead);
			m_OutputBufferSize -= bytesToRead;
			return bytesToRead;
		}
	}

	if (m_FinalBlockProcessed)
	{
		return Length - bytesToRead;
	}

	nLen amountRead;
	nLen outputBytes;

	if (bytesToRead > m_OutputBlockSize)
	{
		if (m_Processor->CanProcessMultiBlocks())
		{
			const auto blocksToProcess = bytesToRead / m_OutputBlockSize;
			const auto inputSize = blocksToProcess * m_InputBlockSize;

			std::vector<nByte> tmpInputBuffer(inputSize);
			std::memmove(tmpInputBuffer.data(), m_InputBuffer.data(), m_InputBufferSize);
			amountRead = m_InputBufferSize;
			amountRead += m_InternalStream->ReadBytes(tmpInputBuffer.data() + m_InputBufferSize, inputSize - m_InputBufferSize);

			m_InputBufferSize = 0;
			if (amountRead <= m_InputBlockSize)
			{
				m_InputBuffer = move(tmpInputBuffer);
				m_InputBufferSize = amountRead;
				goto Normal;
			}

			const auto totalInputSize = amountRead - amountRead % m_InputBlockSize;
			const auto ignoredBytes = amountRead - totalInputSize;

			if (ignoredBytes)
			{
				std::memmove(m_InputBuffer.data(), tmpInputBuffer.data() + totalInputSize, ignoredBytes);
				m_InputBufferSize = ignoredBytes;
			}

			std::vector<nByte> tmpOutputBuffer(totalInputSize / m_InputBlockSize * m_OutputBlockSize);
			tie(std::ignore, outputBytes) = m_Processor->Process(tmpInputBuffer.data(), totalInputSize, tmpOutputBuffer.data(), tmpOutputBuffer.size());
			std::memmove(pWrite, tmpOutputBuffer.data(), outputBytes);
			bytesToRead -= outputBytes;
			pWrite += outputBytes;
		}
	}

Normal:
	while (bytesToRead)
	{
		while (m_InputBufferSize < m_InputBlockSize)
		{
			amountRead = m_InternalStream->ReadBytes(m_InputBuffer.data() + m_InputBufferSize, m_InputBlockSize - m_InputBufferSize);
			if (!amountRead || m_InternalStream->IsEndOfStream())
			{
				goto ProcessFinalBlock;
			}
			m_InputBufferSize += amountRead;
		}
		tie(std::ignore, outputBytes) = m_Processor->Process(m_InputBuffer.data(), m_InputBlockSize, m_OutputBuffer.data(), m_OutputBuffer.size());
		m_InputBufferSize = 0;
		if (bytesToRead >= outputBytes)
		{
			std::memmove(pWrite, m_OutputBuffer.data(), outputBytes);
			bytesToRead -= outputBytes;
			pWrite += outputBytes;
		}
		else
		{
			std::memmove(pWrite, m_OutputBuffer.data(), bytesToRead);
			m_OutputBufferSize -= outputBytes - bytesToRead;
			std::memmove(m_OutputBuffer.data(), m_OutputBuffer.data() + bytesToRead, m_OutputBufferSize);
			return Length;
		}
	}
	return Length;

ProcessFinalBlock:
	tie(std::ignore, m_OutputBuffer) = m_Processor->ProcessFinal(m_InputBuffer.data(), m_InputBufferSize);
	m_OutputBufferSize = m_OutputBuffer.size();
	m_FinalBlockProcessed = true;

	if (bytesToRead < m_OutputBufferSize)
	{
		std::memmove(pWrite, m_OutputBuffer.data(), bytesToRead);
		m_OutputBufferSize -= bytesToRead;
		std::memmove(m_OutputBuffer.data(), m_OutputBuffer.data() + bytesToRead, m_OutputBufferSize);
		return Length;
	}

	std::memmove(pWrite, m_OutputBuffer.data(), m_OutputBufferSize);
	bytesToRead -= m_OutputBufferSize;
	m_OutputBufferSize = 0;
	return Length - bytesToRead;
}

nLen natCryptoStream::WriteBytes(ncData pData, nLen Length)
{
	assert(!m_InputBufferSize || m_InputBlockSize <= m_InputBufferSize);

	if (m_IsReadMode)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "This stream cannot be written."_nv);
	}

	auto bytesToWrite = Length;
	auto pRead = pData;

	if (m_InputBufferSize)
	{
		if (bytesToWrite > m_InputBlockSize - m_InputBufferSize)
		{
			const auto needBytes = m_InputBlockSize - m_InputBufferSize;
			std::memmove(m_InputBuffer.data() + m_InputBufferSize, pRead, needBytes);
			pRead += needBytes;
			bytesToWrite -= needBytes;
		}
		else
		{
			std::memmove(m_InputBuffer.data() + m_InputBufferSize, pRead, bytesToWrite);
			m_InputBufferSize += bytesToWrite;
			return bytesToWrite;
		}
	}

	nLen writtenBytes = 0, currentWrittenBytes;

	if (m_OutputBufferSize)
	{
		writtenBytes += m_InternalStream->WriteBytes(m_OutputBuffer.data(), m_OutputBufferSize);
		m_OutputBufferSize = 0;
	}

	if (m_InputBlockSize == m_InputBufferSize)
	{
		tie(std::ignore, currentWrittenBytes) = m_Processor->Process(m_InputBuffer.data(), m_InputBlockSize, m_OutputBuffer.data(), m_OutputBuffer.size());
		writtenBytes += m_InternalStream->WriteBytes(m_OutputBuffer.data(), currentWrittenBytes);
		m_InputBufferSize = 0;
	}

	const auto canProcessMultiBlocks = m_Processor->CanProcessMultiBlocks();

	while (bytesToWrite)
	{
		if (bytesToWrite > m_InputBlockSize)
		{
			if (canProcessMultiBlocks)
			{
				const auto blocksToProcess = bytesToWrite / m_InputBlockSize;
				const auto inputSize = blocksToProcess * m_InputBlockSize;

				std::vector<nByte> tmpOutputBuffer(blocksToProcess * m_OutputBlockSize);
				tie(std::ignore, currentWrittenBytes) = m_Processor->Process(pRead, inputSize, tmpOutputBuffer.data(), tmpOutputBuffer.size());

				writtenBytes += m_InternalStream->WriteBytes(tmpOutputBuffer.data(), currentWrittenBytes);

				pRead += inputSize;
				bytesToWrite -= inputSize;
			}
			else
			{
				tie(std::ignore, currentWrittenBytes) = m_Processor->Process(pRead, m_InputBlockSize, m_OutputBuffer.data(), m_OutputBuffer.size());

				writtenBytes += m_InternalStream->WriteBytes(m_OutputBuffer.data(), currentWrittenBytes);

				pRead += m_InputBlockSize;
				bytesToWrite -= m_InputBlockSize;
			}
		}
		else
		{
			std::memmove(m_InputBuffer.data(), pRead, bytesToWrite);
			m_InputBufferSize += bytesToWrite;
			break;
		}
	}

	return writtenBytes;
}

void natCryptoStream::FlushFinalBlock()
{
	if (m_FinalBlockProcessed)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Final block has been processed."_nv);
	}

	const auto finalBytes = m_Processor->ProcessFinal(m_InputBuffer.data(), m_InputBufferSize).second;
	m_FinalBlockProcessed = true;

	if (!m_IsReadMode)
	{
		if (m_OutputBufferSize)
		{
			m_InternalStream->WriteBytes(m_OutputBuffer.data(), m_OutputBufferSize);
			m_OutputBufferSize = 0;
		}
		
		m_InternalStream->WriteBytes(finalBytes.data(), finalBytes.size());
	}

	const auto innerCryptoStream = static_cast<natRefPointer<natCryptoStream>>(m_InternalStream);
	if (innerCryptoStream)
	{
		if (!innerCryptoStream->m_FinalBlockProcessed)
		{
			innerCryptoStream->FlushFinalBlock();
		}
	}
	else
	{
		m_InternalStream->Flush();
	}
}
