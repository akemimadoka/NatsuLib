#include "stdafx.h"
#include "natBinary.h"

using namespace NatsuLib;

natBinaryReader::natBinaryReader(natRefPointer<natStream> stream, Environment::Endianness endianness) noexcept
	: m_Stream{ std::move(stream) }, m_Endianness{ endianness }, m_NeedSwapEndian{ endianness != Environment::GetEndianness() }
{
}

natBinaryReader::~natBinaryReader()
{
}

natRefPointer<natStream> natBinaryReader::GetUnderlyingStream() const noexcept
{
	return m_Stream;
}

Environment::Endianness natBinaryReader::GetEndianness() const noexcept
{
	return m_Endianness;
}

void natBinaryReader::Skip(nLen bytes)
{
	if (!bytes)
	{
		return;
	}

	if (m_Stream->CanSeek())
	{
		m_Stream->SetPosition(NatSeek::Cur, static_cast<nLong>(bytes));
	}
	else
	{
		auto remainedBytes = bytes;
		std::vector<nByte> buffer(std::min(remainedBytes, std::numeric_limits<size_t>::max()));
		while (remainedBytes)
		{
			m_Stream->ReadBytes(buffer.data(), std::min(remainedBytes, buffer.size()));
			remainedBytes -= buffer.size();
		}
	}
}

natBinaryWriter::natBinaryWriter(natRefPointer<natStream> stream, Environment::Endianness endianness) noexcept
	: m_Stream{ std::move(stream) }, m_Endianness{ endianness }, m_NeedSwapEndian{ endianness != Environment::GetEndianness() }
{
}

natBinaryWriter::~natBinaryWriter()
{
}

natRefPointer<natStream> natBinaryWriter::GetUnderlyingStream() const noexcept
{
	return m_Stream;
}

Environment::Endianness natBinaryWriter::GetEndianness() const noexcept
{
	return m_Endianness;
}
