#include "stdafx.h"
#include "natStream.h"
#include "natException.h"
#include <algorithm>
#include <cstring>

using namespace NatsuLib;

natStream::~natStream()
{
}

nByte natStream::ReadByte()
{
	nByte byte;
	if (ReadBytes(&byte, 1) == 1)
	{
		return byte;
	}

	nat_Throw(natErrException, NatErr_InternalErr, "Unable to read byte."_nv);
}

void natStream::ForceReadBytes(nData pData, nLen Length)
{
	nLen totalReadBytes{};
	do
	{
		const auto currentReadBytes = ReadBytes(pData + totalReadBytes, Length - totalReadBytes);
		if (!currentReadBytes)
		{
			nat_Throw(natErrException, NatErr_InternalErr, "Unexpected end of stream."_nv);
		}
		totalReadBytes += currentReadBytes;
	} while (totalReadBytes < Length);
}

std::future<nLen> natStream::ReadBytesAsync(nData pData, nLen Length)
{
	return std::async(std::launch::async, [=]
	{
		return ReadBytes(pData, Length);
	});
}

void natStream::WriteByte(nByte byte)
{
	if (WriteBytes(&byte, 1) != 1)
	{
		nat_Throw(natErrException, NatErr_InternalErr, "Unable to write byte."_nv);
	}
}

void natStream::ForceWriteBytes(ncData pData, nLen Length)
{
	nLen totalWrittenBytes{};
	do
	{
		const auto currentWrittenBytes = WriteBytes(pData + totalWrittenBytes, Length - totalWrittenBytes);
		if (!currentWrittenBytes)
		{
			nat_Throw(natErrException, NatErr_InternalErr, "Unexpected end of stream."_nv);
		}
		totalWrittenBytes += currentWrittenBytes;
	} while (totalWrittenBytes < Length);
}

std::future<nLen> natStream::WriteBytesAsync(ncData pData, nLen Length)
{
	return std::async(std::launch::async, [=]
	{
		return WriteBytes(pData, Length);
	});
}

nLen natStream::CopyTo(natRefPointer<natStream> const& other)
{
	assert(other && "other should not be nullptr.");

	nByte buffer[DefaultCopyToBufferSize];
	nLen readBytes, totalReadBytes{};
	while (!IsEndOfStream())
	{
		readBytes = ReadBytes(buffer, sizeof buffer);
		totalReadBytes += readBytes;

		if (readBytes == 0)
		{
			break;
		}

		other->WriteBytes(buffer, readBytes);
	}

	return totalReadBytes;
}

#ifdef _WIN32
natFileStream::natFileStream(nStrView filename, nBool bReadable, nBool bWritable, nBool isAsync)
	: m_hMappedFile(NULL), m_ShouldDispose(true), m_IsAsync(isAsync), m_Filename(filename), m_bReadable(bReadable), m_bWritable(bWritable)
{
	m_hFile = CreateFile(
#ifdef UNICODE
	WideString{filename}.data()
#else
	AnsiString{filename}.data()
#endif
	,
		(bReadable ? GENERIC_READ : 0) | (bWritable ? GENERIC_WRITE : 0),
		FILE_SHARE_READ,
		NULL,
		bWritable ? OPEN_ALWAYS : OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | (isAsync ? FILE_FLAG_OVERLAPPED : 0),
		NULL
	);

	if (!m_hFile || m_hFile == INVALID_HANDLE_VALUE)
	{
		nat_Throw(natWinException, "Open file \"{0}\" failed"_nv, filename);
	}
}

natFileStream::natFileStream(UnsafeHandle hFile, nBool bReadable, nBool bWritable, nBool transferOwner, nBool isAsync)
	: m_hFile(hFile), m_hMappedFile(NULL), m_ShouldDispose(transferOwner), m_IsAsync(isAsync), m_bReadable(bReadable), m_bWritable(bWritable)
{
	if (!m_hFile || m_hFile == INVALID_HANDLE_VALUE)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "Invalid hFile."_nv);
	}
}

nBool natFileStream::CanWrite() const
{
	return m_bWritable;
}

nBool natFileStream::CanRead() const
{
	return m_bReadable;
}

nBool natFileStream::CanResize() const
{
	return m_bWritable;
}

nBool natFileStream::CanSeek() const
{
	return true;
}

nBool natFileStream::IsEndOfStream() const
{
	return GetSize() == GetPosition();
}

nLen natFileStream::GetSize() const
{
	return GetFileSize(m_hFile, NULL);
}

void natFileStream::SetSize(nLen Size)
{
	if (!m_bWritable)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not writable."_nv);
	}

	LARGE_INTEGER tVal;
	tVal.QuadPart = Size;

	if (SetFilePointer(m_hFile, tVal.LowPart, &tVal.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		nat_Throw(natWinException, "SetFilePointer failed."_nv);
	}

	if (SetEndOfFile(m_hFile) == FALSE)
	{
		nat_Throw(natWinException, "SetEndOfFile failed."_nv);
	}

	SetPosition(NatSeek::Beg, 0l);
}

nLen natFileStream::GetPosition() const
{
	LARGE_INTEGER tVal;
	tVal.QuadPart = 0;

	tVal.LowPart = SetFilePointer(m_hFile, tVal.LowPart, &tVal.HighPart, FILE_CURRENT);
	return tVal.QuadPart;
}

void natFileStream::SetPosition(NatSeek Origin, nLong Offset)
{
	nuInt tOrigin;
	switch (Origin)
	{
	case NatSeek::Beg:
		tOrigin = FILE_BEGIN;
		break;
	case NatSeek::Cur:
		tOrigin = FILE_CURRENT;
		break;
	case NatSeek::End:
		tOrigin = FILE_END;
		break;
	default:
		nat_Throw(natErrException, NatErr_InvalidArg, "Origin is not a valid NatSeek."_nv);
	}

	LARGE_INTEGER tVal;
	tVal.QuadPart = Offset;

	if (SetFilePointer(m_hFile, tVal.LowPart, &tVal.HighPart, tOrigin) == INVALID_SET_FILE_POINTER)
	{
		nat_Throw(natWinException, "SetFilePointer failed."_nv);
	}
}

nByte natFileStream::ReadByte()
{
	nByte byte;
	if (ReadBytes(&byte, 1) == 1)
	{
		return byte;
	}

	nat_Throw(natErrException, NatErr_InternalErr, "Unable to read byte."_nv);
}

nLen natFileStream::ReadBytes(nData pData, nLen Length)
{
	if (m_IsAsync)
	{
		return ReadBytesAsync(pData, Length).get();
	}

	DWORD tReadBytes = 0ul;
	if (!m_bReadable)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not readable."_nv);
	}

	if (Length == 0ul)
	{
		return tReadBytes;
	}

	if (pData == nullptr)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "pData cannot be nullptr."_nv);
	}

	if (ReadFile(m_hFile, pData, static_cast<DWORD>(Length), &tReadBytes, NULL) == FALSE)
	{
		nat_Throw(natWinException, "ReadFile failed."_nv);
	}

	return tReadBytes;
}

void natFileStream::WriteByte(nByte byte)
{
	if (WriteBytes(&byte, 1) != 1)
	{
		nat_Throw(natErrException, NatErr_InternalErr, "Unable to write byte."_nv);
	}
}

nLen natFileStream::WriteBytes(ncData pData, nLen Length)
{
	if (m_IsAsync)
	{
		return WriteBytesAsync(pData, Length).get();
	}

	DWORD tWriteBytes = 0ul;
	if (!m_bWritable)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not writable."_nv);
	}

	if (Length == 0ul)
	{
		return tWriteBytes;
	}

	if (pData == nullptr)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "pData cannot be nullptr."_nv);
	}

	if (WriteFile(m_hFile, pData, static_cast<DWORD>(Length), &tWriteBytes, NULL) == FALSE)
	{
		nat_Throw(natWinException, "WriteFile failed."_nv);
	}

	return tWriteBytes;
}

std::future<nLen> natFileStream::ReadBytesAsync(nData pData, nLen Length)
{
	if (m_IsAsync)
	{
		if (!m_bReadable)
		{
			nat_Throw(natErrException, NatErr_IllegalState, "Stream is not readable."_nv);
		}

		if (Length == 0ul)
		{
			std::promise<nLen> dummyPromise;
			dummyPromise.set_value(0);
			return dummyPromise.get_future();
		}

		if (pData == nullptr)
		{
			nat_Throw(natErrException, NatErr_InvalidArg, "pData cannot be nullptr."_nv);
		}

		auto olp = std::make_unique<OVERLAPPED>();

		if (ReadFile(m_hFile, pData, static_cast<DWORD>(Length), NULL, olp.get()) == FALSE)
		{
			auto lastError = GetLastError();
			if (lastError != ERROR_IO_PENDING)
			{
				nat_Throw(natWinException, lastError, "ReadFile failed."_nv);
			}
		}

		return std::async(std::launch::deferred, [olp = move(olp), this]() -> nLen
		{
			DWORD tReadBytes;
			if (!GetOverlappedResult(m_hFile, olp.get(), &tReadBytes, TRUE))
			{
				nat_Throw(natWinException, "GetOverlappedResult failed."_nv);
			}

			return tReadBytes;
		});
	}

	return std::async(std::launch::async, [=]
	{
		return ReadBytes(pData, Length);
	});
}

std::future<nLen> natFileStream::WriteBytesAsync(ncData pData, nLen Length)
{
	if (m_IsAsync)
	{
		if (!m_bWritable)
		{
			nat_Throw(natErrException, NatErr_IllegalState, "Stream is not writable."_nv);
		}

		if (Length == 0ul)
		{
			std::promise<nLen> dummyPromise;
			dummyPromise.set_value(0);
			return dummyPromise.get_future();
		}

		if (pData == nullptr)
		{
			nat_Throw(natErrException, NatErr_InvalidArg, "pData cannot be nullptr."_nv);
		}

		auto olp = std::make_unique<OVERLAPPED>();

		if (WriteFile(m_hFile, pData, static_cast<DWORD>(Length), NULL, olp.get()) == FALSE)
		{
			auto lastError = GetLastError();
			if (lastError != ERROR_IO_PENDING)
			{
				nat_Throw(natWinException, lastError, "WriteFile failed."_nv);
			}
		}

		return std::async(std::launch::deferred, [olp = move(olp), this]() -> nLen
		{
			DWORD tWrittenBytes;
			if (!GetOverlappedResult(m_hFile, olp.get(), &tWrittenBytes, TRUE))
			{
				nat_Throw(natWinException, "GetOverlappedResult failed."_nv);
			}

			return tWrittenBytes;
		});
	}

	return std::async(std::launch::async, [=]
	{
		return WriteBytes(pData, Length);
	});
}

void natFileStream::Flush()
{
	if (m_pMappedFile)
	{
		FlushViewOfFile(m_pMappedFile->GetInternalBuffer(), static_cast<SIZE_T>(GetSize()));
	}

	FlushFileBuffers(m_hFile);
}

nStrView natFileStream::GetFilename() const noexcept
{
	return m_Filename;
}

natFileStream::UnsafeHandle natFileStream::GetUnsafeHandle() const noexcept
{
	return m_hFile;
}

natRefPointer<natMemoryStream> natFileStream::MapToMemoryStream()
{
	if (m_pMappedFile)
	{
		return m_pMappedFile;
	}

	m_hMappedFile = CreateFileMapping(m_hFile, NULL, m_bWritable ? PAGE_READWRITE : PAGE_READONLY, NULL, NULL, NULL);
	if (!m_hMappedFile || m_hMappedFile == INVALID_HANDLE_VALUE)
	{
		nat_Throw(natWinException, "CreateFileMapping failed."_nv);
	}

	auto pFile = MapViewOfFile(m_hMappedFile, (m_bReadable ? FILE_MAP_READ : 0) | (m_bWritable ? FILE_MAP_WRITE : 0), NULL, NULL, NULL);
	if (!pFile)
	{
		nat_Throw(natWinException, "MapViewOfFile failed."_nv);
	}

	m_pMappedFile = make_ref<natExternMemoryStream>(static_cast<nData>(pFile), GetSize(), m_bReadable, m_bWritable);

	return m_pMappedFile;
}

natSubStream::natSubStream(natRefPointer<natStream> stream, nLen startPosition, nLen endPosition)
	: m_InternalStream{ std::move(stream) }, m_StartPosition{ startPosition }, m_EndPosition{ endPosition }, m_CurrentPosition{ startPosition }
{
	if (!m_InternalStream->CanSeek())
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "stream should be seekable."_nv);
	}
	if (startPosition > endPosition)
	{
		nat_Throw(natErrException, NatErr_OutOfRange, "startPosition cannot be bigger than endPosition."_nv);
	}
	if (m_InternalStream->GetSize() < endPosition)
	{
		nat_Throw(natErrException, NatErr_OutOfRange, "Range is too big."_nv);
	}
	m_InternalStream->SetPosition(NatSeek::Beg, startPosition);
}

natSubStream::~natSubStream()
{
}

natRefPointer<natStream> natSubStream::GetUnderlyingStream() const noexcept
{
	return m_InternalStream;
}

nBool natSubStream::CanWrite() const
{
	return m_InternalStream->CanWrite();
}

nBool natSubStream::CanRead() const
{
	return m_InternalStream->CanRead();
}

nBool natSubStream::CanResize() const
{
	return false;
}

nBool natSubStream::CanSeek() const
{
	return true;
}

nBool natSubStream::IsEndOfStream() const
{
	return m_CurrentPosition == m_EndPosition;
}

nLen natSubStream::GetSize() const
{
	return m_EndPosition - m_StartPosition;
}

void natSubStream::SetSize(nLen /*Size*/)
{
	nat_Throw(natErrException, NatErr_NotSupport, "This type of stream does not support SetSize."_nv);
}

nLen natSubStream::GetPosition() const
{
	return m_CurrentPosition;
}

void natSubStream::SetPosition(NatSeek Origin, nLong Offset)
{
	nLen position{};
	switch (Origin)
	{
	case NatSeek::Beg:
		position = m_StartPosition + Offset;
		break;
	case NatSeek::Cur:
		position = m_CurrentPosition + Offset;
		break;
	case NatSeek::End:
		position = m_EndPosition + Offset;
		break;
	default:
		assert(!"Invalid Origin.");
	}

	if (position < m_StartPosition || position > m_EndPosition)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "Out of range."_nv);
	}

	m_CurrentPosition = position;

	adjustPosition();
}

nByte natSubStream::ReadByte()
{
	if (!m_InternalStream->CanRead())
	{
		nat_Throw(natErrException, NatErr_NotSupport, "Underlying stream cannot read."_nv);
	}

	if (m_CurrentPosition >= m_EndPosition)
	{
		nat_Throw(natErrException, NatErr_OutOfRange, "Reached end of stream."_nv);
	}
	
	adjustPosition();
	return m_InternalStream->ReadByte();
}

nLen natSubStream::ReadBytes(nData pData, nLen Length)
{
	if (!m_InternalStream->CanRead())
	{
		nat_Throw(natErrException, NatErr_NotSupport, "Underlying stream cannot read."_nv);
	}

	const auto realLength = std::min(Length, m_EndPosition - m_CurrentPosition);
	adjustPosition();
	return m_InternalStream->ReadBytes(pData, realLength);
}

std::future<nLen> natSubStream::ReadBytesAsync(nData pData, nLen Length)
{
	if (!m_InternalStream->CanRead())
	{
		nat_Throw(natErrException, NatErr_NotSupport, "Underlying stream cannot read."_nv);
	}

	const auto realLength = std::min(Length, m_EndPosition - m_CurrentPosition);
	adjustPosition();
	return m_InternalStream->ReadBytesAsync(pData, realLength);
}

void natSubStream::WriteByte(nByte byte)
{
	if (!m_InternalStream->CanWrite())
	{
		nat_Throw(natErrException, NatErr_NotSupport, "Underlying stream cannot write."_nv);
	}

	if (m_CurrentPosition >= m_EndPosition)
	{
		nat_Throw(natErrException, NatErr_OutOfRange, "Reached end of stream."_nv);
	}

	adjustPosition();
	return m_InternalStream->WriteByte(byte);
}

nLen natSubStream::WriteBytes(ncData pData, nLen Length)
{
	if (!m_InternalStream->CanWrite())
	{
		nat_Throw(natErrException, NatErr_NotSupport, "Underlying stream cannot write."_nv);
	}

	const auto realLength = std::min(Length, m_EndPosition - m_CurrentPosition);
	adjustPosition();
	return m_InternalStream->WriteBytes(pData, realLength);
}

std::future<nLen> natSubStream::WriteBytesAsync(ncData pData, nLen Length)
{
	if (!m_InternalStream->CanWrite())
	{
		nat_Throw(natErrException, NatErr_NotSupport, "Underlying stream cannot write."_nv);
	}

	const auto realLength = std::min(Length, m_EndPosition - m_CurrentPosition);
	adjustPosition();
	return m_InternalStream->WriteBytesAsync(pData, realLength);
}

void natSubStream::Flush()
{
	m_InternalStream->Flush();
}

void natSubStream::adjustPosition() const
{
	assert(m_CurrentPosition >= m_StartPosition && m_CurrentPosition <= m_EndPosition);
	
	if (m_CurrentPosition == m_InternalStream->GetPosition())
	{
		return;
	}

	if (m_CurrentPosition <= static_cast<nLen>(std::numeric_limits<nLong>::max()))
	{
		m_InternalStream->SetPosition(NatSeek::Beg, m_CurrentPosition);
	}
	else
	{
		m_InternalStream->SetPosition(NatSeek::End, -static_cast<nLong>(std::numeric_limits<nLen>::max() - m_CurrentPosition));
	}
}

void natSubStream::checkPosition() const
{
	if (m_CurrentPosition < m_StartPosition || m_CurrentPosition > m_EndPosition)
	{
		nat_Throw(natErrException, NatErr_OutOfRange, "m_CurrentPosition is out of range."_nv);
	}

	const auto position = m_InternalStream->GetPosition();

	if (m_CurrentPosition != position)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "m_CurrentPosition does not equal to real position in m_InternalStream."_nv);
	}
}

natFileStream::~natFileStream()
{
	CloseHandle(m_hMappedFile);
	if (m_ShouldDispose)
	{
		CloseHandle(m_hFile);
	}
}

natStdStream::natStdStream(StdStreamType stdStreamType)
	: m_StdStreamType(stdStreamType)
{
	switch (m_StdStreamType)
	{
	case StdIn:
		m_StdHandle = GetStdHandle(STD_INPUT_HANDLE);
		break;
	case StdOut:
		m_StdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		break;
	case StdErr:
		m_StdHandle = GetStdHandle(STD_ERROR_HANDLE);
		break;
	default:
		nat_Throw(natException, "Invalid StdStreamType."_nv);
	}

	if (!m_StdHandle || m_StdHandle == INVALID_HANDLE_VALUE)
	{
		nat_Throw(natWinException, "GetStdHandle failed."_nv);
	}

	// 当控制台代码页不是Unicode或者被重定向的时候我们应当使用文件API
	// 由于natConsole被钦定编码，所以在此我们只判断是否重定向
	DWORD consoleMode;
	if (!(GetFileType(m_StdHandle) & FILE_TYPE_CHAR) || !GetConsoleMode(m_StdHandle, &consoleMode))
	{
		m_InternalStream = make_ref<natFileStream>(m_StdHandle, m_StdStreamType == StdIn, m_StdStreamType != StdIn, false);
	}
}

nBool natStdStream::UseFileApi() const noexcept
{
	return m_InternalStream != nullptr;
}

natStdStream::~natStdStream()
{
}

nBool natStdStream::CanWrite() const
{
	return m_StdStreamType != StdIn;
}

nBool natStdStream::CanRead() const
{
	return m_StdStreamType == StdIn;
}

nBool natStdStream::CanResize() const
{
	return false;
}

nBool natStdStream::CanSeek() const
{
	return false;
}

nBool natStdStream::IsEndOfStream() const
{
	return false;
}

nLen natStdStream::GetSize() const
{
	return 0;
}

void natStdStream::SetSize(nLen)
{
	nat_Throw(natErrException, NatErr_NotSupport, "This stream cannot set size."_nv);
}

nLen natStdStream::GetPosition() const
{
	return 0;
}

void natStdStream::SetPosition(NatSeek, nLong)
{
	nat_Throw(natErrException, NatErr_NotSupport, "This stream cannot set position."_nv);
}

nByte natStdStream::ReadByte()
{
	if (m_InternalStream)
	{
		return m_InternalStream->ReadByte();
	}
	
	nByte byte;
	if (ReadBytes(&byte, 1) == 1)
	{
		return byte;
	}

	nat_Throw(natErrException, NatErr_InternalErr, "Cannot read byte."_nv);
}

nLen natStdStream::ReadBytes(nData pData, nLen Length)
{
	if (m_InternalStream)
	{
		return m_InternalStream->ReadBytes(pData, Length);
	}
	
	// Workaround: 辣鸡WinAPI
	if (!CanRead())
	{
		nat_Throw(natErrException, NatErr_IllegalState, "This stream cannot read."_nv);
	}

	DWORD readCharsCount;
	if (!ReadConsole(m_StdHandle, pData, static_cast<DWORD>(Length / sizeof(TCHAR)), &readCharsCount, NULL))
	{
		nat_Throw(natWinException, "ReadConsole failed."_nv);
	}

	return static_cast<nLen>(readCharsCount * sizeof(TCHAR));
}

std::future<nLen> natStdStream::ReadBytesAsync(nData pData, nLen Length)
{
	if (m_InternalStream)
	{
		return m_InternalStream->ReadBytesAsync(pData, Length);
	}
	
	return std::async(std::launch::async, [=]
	{
		return ReadBytes(pData, Length);
	});
}

void natStdStream::WriteByte(nByte byte)
{
	if (m_InternalStream)
	{
		m_InternalStream->WriteByte(byte);
	}
	
	if (WriteBytes(&byte, 1) != 1)
	{
		nat_Throw(natWinException, "Cannot write byte."_nv);
	}
}

nLen natStdStream::WriteBytes(ncData pData, nLen Length)
{
	if (m_InternalStream)
	{
		return m_InternalStream->WriteBytes(pData, Length);
	}
	
	// Workaround: 辣鸡WinAPI
	if (!CanWrite())
	{
		nat_Throw(natErrException, NatErr_IllegalState, "This stream cannot write."_nv);
	}

	DWORD writtenCharsCount;
	if (!WriteConsole(m_StdHandle, pData, static_cast<DWORD>(Length / sizeof(TCHAR)), &writtenCharsCount, NULL))
	{
		nat_Throw(natWinException, "WriteConsole failed."_nv);
	}

	return static_cast<nLen>(writtenCharsCount * sizeof(TCHAR));
}

std::future<nLen> natStdStream::WriteBytesAsync(ncData pData, nLen Length)
{
	if (m_InternalStream)
	{
		return m_InternalStream->WriteBytesAsync(pData, Length);
	}

	return std::async(std::launch::async, [=]
	{
		return WriteBytes(pData, Length);
	});
}

void natStdStream::Flush()
{
	if (m_InternalStream)
	{
		m_InternalStream->Flush();
		return;
	}
	
	if (CanRead())
	{
		FlushConsoleInputBuffer(m_StdHandle);
	}
}
#else
natFileStream::natFileStream(nStrView filename, nBool bReadable, nBool bWritable)
	: m_CurrentPos(0), m_Filename(filename), m_bReadable(bReadable), m_bWritable(bWritable)
{
	std::ios_base::openmode openmode{};
	if (bReadable)
	{
		openmode |= std::ios_base::in;
	}
	if (bWritable)
	{
		openmode |= std::ios_base::out;
	}

	m_File.open(filename.data(), openmode);
	if (!m_File.is_open())
	{
		nat_Throw(natErrException, NatErr_InternalErr, "Cannot open file \"{0}\"."_nv, filename);
	}

	auto current = m_File.tellg();
	m_File.seekg(0, std::ios_base::end);
	m_Size = static_cast<nLen>(m_File.tellg() - current);
	m_File.seekg(current);
}

nBool natFileStream::CanWrite() const
{
	return m_bWritable;
}

nBool natFileStream::CanRead() const
{
	return m_bReadable;
}

nBool natFileStream::CanResize() const
{
	return m_bWritable;
}

nBool natFileStream::CanSeek() const
{
	return true;
}

nBool natFileStream::IsEndOfStream() const
{
	return m_File.eof();
}

nLen natFileStream::GetSize() const
{
	return m_Size;
}

void natFileStream::SetSize(nLen Size)
{
	if (!m_bWritable)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not writable."_nv);
	}

	if (Size < m_Size)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "Cannot truncate file."_nv);
	}

	if (Size == m_Size)
	{
		return;
	}

	auto current = m_File.tellp();
	m_File.seekp(Size);
	m_File.seekp(current);
	m_Size = Size;
}

nLen natFileStream::GetPosition() const
{
	return m_CurrentPos;
}

void natFileStream::SetPosition(NatSeek Origin, nLong Offset)
{
	std::ios_base::seekdir tOrigin;
	switch (Origin)
	{
	case NatSeek::Beg:
		tOrigin = std::ios_base::beg;
		break;
	case NatSeek::Cur:
		tOrigin = std::ios_base::cur;
		break;
	case NatSeek::End:
		tOrigin = std::ios_base::end;
		break;
	default:
		nat_Throw(natErrException, NatErr_InvalidArg, "Origin is not a valid NatSeek."_nv);
	}

	m_File.seekp(Offset, tOrigin);
	m_CurrentPos = static_cast<nLen>(m_File.tellp());
}

nByte natFileStream::ReadByte()
{
	// 可能有bug？
	return static_cast<nByte>(m_File.get());
}

nLen natFileStream::ReadBytes(nData pData, nLen Length)
{
	if (!m_bReadable)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not readable."_nv);
	}

	if (Length == 0ul)
	{
		return 0;
	}

	if (pData == nullptr)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "pData cannot be nullptr."_nv);
	}

	auto size = m_File.readsome(reinterpret_cast<char*>(pData), static_cast<std::streamsize>(Length));
	if (m_File.fail())
	{
		nat_Throw(natErrException, NatErr_InternalErr, "Reading file failed after readed {0} bytes."_nv, size);
	}

	m_CurrentPos = static_cast<nLen>(m_File.tellg());
	return size;
}

void natFileStream::WriteByte(nByte byte)
{
	m_File.put(static_cast<char>(byte));
}

nLen natFileStream::WriteBytes(ncData pData, nLen Length)
{
	if (!m_bWritable)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not writable."_nv);
	}

	if (Length == 0ul)
	{
		return 0;
	}

	if (pData == nullptr)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "pData cannot be nullptr."_nv);
	}

	m_File.write(reinterpret_cast<const char*>(pData), static_cast<std::streamsize>(Length));
	auto current = m_File.tellp();
	auto size = current - static_cast<std::streamoff>(m_CurrentPos);

	if (m_File.fail())
	{
		nat_Throw(natErrException, NatErr_InternalErr, "Writing file failed after wrote {0} bytes."_nv, size);
	}

	m_CurrentPos = static_cast<nLen>(current);
	return size;
}

void natFileStream::Flush()
{
	m_File.flush();
}

nStrView natFileStream::GetFilename() const noexcept
{
	return m_Filename;
}

natFileStream::~natFileStream()
{
}

natStdStream::natStdStream(StdStreamType stdStreamType)
	: m_StdStreamType(stdStreamType)
{
	switch (m_StdStreamType)
	{
	case StdIn:
		m_StdHandle = stdin;
		break;
	case StdOut:
		m_StdHandle = stdout;
		break;
	case StdErr:
		m_StdHandle = stderr;
		break;
	default:
		nat_Throw(natException, "Invalid StdStreamType."_nv);
	}
}

natStdStream::~natStdStream()
{
}

nByte natStdStream::ReadByte()
{
	assert(CanRead());

	nByte byteToRead;
	if (ReadBytes(&byteToRead, 1) == 1)
	{
		return byteToRead;
	}

	nat_Throw(natErrException, NatErr_InternalErr, "Failed to read a byte."_nv);
}

nLen natStdStream::ReadBytes(nData pData, nLen Length)
{
	if (!CanRead())
	{
		nat_Throw(natErrException, NatErr_IllegalState, "This stream cannot read."_nv);
	}

	/*auto pWrite = pData;
	nLen readBytes{};

	while (readBytes < Length)
	{
		nByte byteToRead;
		if (
#ifdef _MSC_VER
			fread_s(&byteToRead, Length, 1, 1, m_StdHandle)
#else
			fread(&byteToRead, 1, 1, m_StdHandle)
#endif
			== 1)
		{
			*pWrite++ = byteToRead;
			++readBytes;
		}
		else
		{
			break;
		}
	}

	return readBytes;*/
#ifdef _MSC_VER
	return static_cast<nLen>(fread_s(pData, Length, 1, Length, m_StdHandle));
#else
	return static_cast<nLen>(fread(pData, 1, Length, m_StdHandle));
#endif
}

std::future<nLen> natStdStream::ReadBytesAsync(nData pData, nLen Length)
{
	return std::async(std::launch::async, [=]()
	{
		return ReadBytes(pData, Length);
	});
}

void natStdStream::WriteByte(nByte byte)
{
	assert(CanWrite());
	WriteBytes(&byte, 1);
}

nLen natStdStream::WriteBytes(ncData pData, nLen Length)
{
	if (!CanWrite())
	{
		nat_Throw(natErrException, NatErr_IllegalState, "This stream cannot write."_nv);
	}

	return static_cast<nLen>(fwrite(pData, Length, 1, m_StdHandle));
}

std::future<nLen> natStdStream::WriteBytesAsync(ncData pData, nLen Length)
{
	return std::async(std::launch::async, [=]()
	{
		return WriteBytes(pData, Length);
	});
}

void natStdStream::Flush()
{
	fflush(m_StdHandle);
}

#endif

natMemoryStream::natMemoryStream(ncData pData, nLen Length, nBool bReadable, nBool bWritable, nBool autoResize)
	: m_pData(nullptr), m_CurPos(0u), m_bReadable(bReadable), m_bWritable(bWritable), m_AutoResize(autoResize)
{
	Reserve(Length);
	m_Size = Length;
	if (pData)
	{
		memmove(m_pData, pData, Length);
	}
}

natMemoryStream::natMemoryStream(nLen Length, nBool bReadable, nBool bWritable, nBool autoResize)
	: natMemoryStream(nullptr, Length, bReadable, bWritable, autoResize)
{
}

natMemoryStream::natMemoryStream(natMemoryStream const& other)
	: natRefObjImpl<natStream>(), m_pData(), m_Size(), m_Capacity(), m_CurPos(), m_bReadable(), m_bWritable(), m_AutoResize()
{
	*this = other;
}

natMemoryStream::natMemoryStream(natMemoryStream&& other) noexcept
	: m_pData(), m_Size(), m_Capacity(), m_CurPos(), m_bReadable(), m_bWritable(), m_AutoResize()
{
	*this = std::move(other);
}

nBool natMemoryStream::CanWrite() const
{
	return m_bWritable;
}

nBool natMemoryStream::CanRead() const
{
	return m_bReadable;
}

nBool natMemoryStream::CanResize() const
{
	return true;
}

nBool natMemoryStream::CanSeek() const
{
	return true;
}

nBool natMemoryStream::IsEndOfStream() const
{
	return m_CurPos >= m_Capacity;
}

nLen natMemoryStream::GetSize() const
{
	return m_Capacity;
}

void natMemoryStream::SetSize(nLen Size)
{
	Reserve(Size);
	m_Size = Size;
	m_CurPos = std::min(m_CurPos, m_Size);
}

nLen natMemoryStream::GetPosition() const
{
	return m_CurPos;
}

void natMemoryStream::SetPosition(NatSeek Origin, nLong Offset)
{
	switch (Origin)
	{
	case NatSeek::Beg:
		if (Offset < 0 || m_Capacity < static_cast<nLen>(Offset))
			nat_Throw(natErrException, NatErr_OutOfRange, "Out of range."_nv);
		m_CurPos = Offset;
		break;
	case NatSeek::Cur:
		if ((Offset < 0 && m_CurPos < static_cast<nLen>(-Offset)) || m_Capacity > static_cast<nLen>(m_CurPos + Offset))
			nat_Throw(natErrException, NatErr_OutOfRange, "Out of range."_nv);
		m_CurPos += Offset;
		break;
	case NatSeek::End:
		if (Offset > 0 || m_Capacity < static_cast<nLen>(-Offset))
			nat_Throw(natErrException, NatErr_OutOfRange, "Out of range."_nv);
		m_CurPos = m_Capacity + Offset;
		break;
	default:
		nat_Throw(natErrException, NatErr_OutOfRange, "Out of range."_nv);
	}
}

nByte natMemoryStream::ReadByte()
{
	if (m_CurPos >= m_Size)
	{
		nat_Throw(OutOfRange);
	}

	return m_pData[m_CurPos++];
}

nLen natMemoryStream::ReadBytes(nData pData, nLen Length)
{
	assert(m_pData && "m_pData should not be nullptr.");

	nLen tReadBytes = 0ul;

	if (!m_bReadable)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not readable."_nv);
	}

	if (Length == 0ul)
	{
		return tReadBytes;
	}

	natRefScopeGuard<natCriticalSection> guard(m_CriSection);

	tReadBytes = std::min(Length, m_Size - m_CurPos);
	memmove(pData + m_CurPos, m_pData, static_cast<size_t>(tReadBytes));
	m_CurPos += tReadBytes;

	return tReadBytes;
}

std::future<nLen> natMemoryStream::ReadBytesAsync(nData pData, nLen Length)
{
	return std::async(std::launch::async, [=]()
	{
		return ReadBytes(pData, Length);
	});
}

void natMemoryStream::WriteByte(nByte byte)
{
	if (m_CurPos >= m_Capacity)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "End of stream reached."_nv);
	}

	m_pData[m_CurPos++] = byte;
}

nLen natMemoryStream::WriteBytes(ncData pData, nLen Length)
{
	assert(m_pData && "m_pData should not be nullptr.");
	nLen tWriteBytes = 0ul;

	if (!m_bWritable)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not writable."_nv);
	}

	if (Length == 0ul)
	{
		return tWriteBytes;
	}

	natRefScopeGuard<natCriticalSection> guard(m_CriSection);

	if (Length > m_Capacity - m_CurPos)
	{
		if (m_AutoResize)
		{
			Reserve(detail_::Grow(Length));
			tWriteBytes = Length;
		}
		else
		{
			tWriteBytes = m_Capacity - m_CurPos;
		}
	}

	memmove(m_pData, pData + m_CurPos, static_cast<size_t>(tWriteBytes));
	m_CurPos += tWriteBytes;

	return tWriteBytes;
}

std::future<nLen> natMemoryStream::WriteBytesAsync(ncData pData, nLen Length)
{
	return std::async(std::launch::async, [=]()
	{
		return WriteBytes(pData, Length);
	});
}

void natMemoryStream::Flush()
{
}

nData natMemoryStream::GetInternalBuffer() noexcept
{
	return m_pData;
}

ncData natMemoryStream::GetInternalBuffer() const noexcept
{
	return m_pData;
}

void natMemoryStream::Reserve(nLen newCapacity)
{
	using std::swap;

	if (newCapacity <= m_Capacity)
	{
		return;
	}

	auto pNewStorage = new nByte[newCapacity];
	const auto deleter = make_scope([&pNewStorage]
	{
		delete[] pNewStorage;
	});

	if (m_Size > 0 && m_pData)
	{
		memmove(pNewStorage, m_pData, m_Size);
	}
	
	swap(m_pData, pNewStorage);
}

nLen natMemoryStream::GetCapacity() const noexcept
{
	return m_Capacity;
}

natMemoryStream::~natMemoryStream()
{
	SafeDelArr(m_pData);
}

natMemoryStream& natMemoryStream::operator=(natMemoryStream const& other)
{
	if (this == &other)
	{
		return *this;
	}

	natRefScopeGuard<natCriticalSection> otherguard(m_CriSection);
	natRefScopeGuard<natCriticalSection> selfguard(other.m_CriSection);

	allocateAndInvalidateOldData(other.m_Size);

	if (other.m_pData)
	{
		memmove(m_pData, other.m_pData, other.m_Size);
	}
	
	m_Size = other.m_Size;
	m_CurPos = other.m_CurPos;
	m_bReadable = other.m_bReadable;
	m_bWritable = other.m_bWritable;
	m_AutoResize = other.m_AutoResize;

	return *this;
}

natMemoryStream& natMemoryStream::operator=(natMemoryStream&& other) noexcept
{
	using std::swap;

	if (this == &other)
	{
		return *this;
	}

	natRefScopeGuard<natCriticalSection> otherguard(m_CriSection);
	natRefScopeGuard<natCriticalSection> selfguard(other.m_CriSection);

	swap(m_pData, other.m_pData);
	swap(m_Size, other.m_Size);
	swap(m_Capacity, other.m_Capacity);
	swap(m_CurPos, other.m_CurPos);
	swap(m_bReadable, other.m_bReadable);
	swap(m_bWritable, other.m_bWritable);
	swap(m_AutoResize, other.m_AutoResize);

	return *this;
}

void natMemoryStream::allocateAndInvalidateOldData(nLen newCapacity)
{
	using std::swap;

	if (newCapacity <= m_Capacity)
	{
		return;
	}

	auto pNewStorage = new nByte[newCapacity];
	swap(m_pData, pNewStorage); // 假设swap总是noexcept的 
	delete[] pNewStorage;
}

natExternMemoryStream::natExternMemoryStream(nData externData, nLen size, nBool readable, nBool writable)
	: m_ExternData{ externData }, m_Size{ size }, m_CurrentPos{}, m_Readable{ readable }, m_Writable{ writable }
{
	if (!externData)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "externData should not be nullptr."_nv);
	}
}

natExternMemoryStream::natExternMemoryStream(ncData externData, nLen size, nBool readable)
	: natExternMemoryStream{ const_cast<nData>(externData), size, readable, false }
{
}

natExternMemoryStream::~natExternMemoryStream()
{
}

nBool natExternMemoryStream::CanWrite() const
{
	return m_Writable;
}

nBool natExternMemoryStream::CanRead() const
{
	return m_Readable;
}

nBool natExternMemoryStream::CanResize() const
{
	return false;
}

nBool natExternMemoryStream::CanSeek() const
{
	return true;
}

nBool natExternMemoryStream::IsEndOfStream() const
{
	assert(m_CurrentPos <= m_Size);
	return m_CurrentPos == m_Size;
}

nLen natExternMemoryStream::GetSize() const
{
	return m_Size;
}

void natExternMemoryStream::SetSize(nLen /*Size*/)
{
	nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
}

nLen natExternMemoryStream::GetPosition() const
{
	return m_CurrentPos;
}

void natExternMemoryStream::SetPosition(NatSeek Origin, nLong Offset)
{
	switch (Origin)
	{
	case NatSeek::Beg:
		if (Offset < 0 || m_Size < static_cast<nLen>(Offset))
			nat_Throw(OutOfRange, "Out of range."_nv);
		m_CurrentPos = Offset;
		break;
	case NatSeek::Cur:
		if ((Offset < 0 && m_CurrentPos < static_cast<nLen>(-Offset)) || m_Size > static_cast<nLen>(m_CurrentPos + Offset))
			nat_Throw(OutOfRange, "Out of range."_nv);
		m_CurrentPos += Offset;
		break;
	case NatSeek::End:
		if (Offset > 0 || m_Size < static_cast<nLen>(-Offset))
			nat_Throw(OutOfRange, "Out of range."_nv);
		m_CurrentPos = m_Size + Offset;
		break;
	default:
		assert(!"Invalid Origin.");
		nat_Throw(OutOfRange, "Invalid Origin."_nv);
	}
}

nByte natExternMemoryStream::ReadByte()
{
	assert(m_CurrentPos <= m_Size);
	if (m_CurrentPos == m_Size)
	{
		nat_Throw(OutOfRange, "End of stream reached."_nv);
	}
	return m_ExternData[m_CurrentPos++];
}

nLen natExternMemoryStream::ReadBytes(nData pData, nLen Length)
{
	assert(pData && "pData should not be nullptr.");
	assert(m_CurrentPos <= m_Size);

	if (!CanRead())
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not readable."_nv);
	}

	if (Length == 0)
	{
		return 0;
	}

	const auto readBytes = std::min(Length, m_Size - m_CurrentPos);
	memmove(pData, m_ExternData, readBytes);
	m_CurrentPos += readBytes;
	return readBytes;
}

void natExternMemoryStream::WriteByte(nByte byte)
{
	assert(m_CurrentPos <= m_Size);
	if (m_CurrentPos == m_Size)
	{
		nat_Throw(OutOfRange, "End of stream reached."_nv);
	}
	m_ExternData[m_CurrentPos++] = byte;
}

nLen natExternMemoryStream::WriteBytes(ncData pData, nLen Length)
{
	assert(pData && "pData should not be nullptr.");
	assert(m_CurrentPos <= m_Size);

	if (!CanRead())
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not readable."_nv);
	}

	if (Length == 0)
	{
		return 0;
	}

	const auto writtenBytes = std::min(Length, m_Size - m_CurrentPos);
	memmove(m_ExternData + m_CurrentPos, pData, writtenBytes);
	m_CurrentPos += writtenBytes;
	return writtenBytes;
}

void natExternMemoryStream::Flush()
{
}

ncData natExternMemoryStream::GetExternData() const noexcept
{
	return m_ExternData;
}
