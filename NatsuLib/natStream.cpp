#include "stdafx.h"
#include "natStream.h"
#include "natException.h"
#include <algorithm>
#include <cstring>

using namespace NatsuLib;

nByte natStream::ReadByte()
{
	nByte byte;
	if (ReadBytes(&byte, 1) == 1)
	{
		return byte;
	}

	nat_Throw(natErrException, NatErr_InternalErr, "Unable to read byte."_nv);
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

std::future<nLen> natStream::WriteBytesAsync(ncData pData, nLen Length)
{
	return std::async(std::launch::async, [=]
	{
		return WriteBytes(pData, Length);
	});
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

		return std::async(std::launch::async, [=]() -> nLen
		{
			auto hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
			OVERLAPPED olp{};
			olp.hEvent = hEvent;

			if (ReadFile(m_hFile, pData, static_cast<DWORD>(Length), NULL, &olp) == FALSE)
			{
				auto lastError = GetLastError();
				if (lastError != ERROR_IO_PENDING)
				{
					nat_Throw(natWinException, lastError, "ReadFile failed."_nv);
				}
			}

			DWORD tReadBytes;
			if (!GetOverlappedResult(m_hFile, &olp, &tReadBytes, TRUE))
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

		return std::async(std::launch::async, [=]() -> nLen
		{
			auto hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
			OVERLAPPED olp{};
			olp.hEvent = hEvent;

			if (WriteFile(m_hFile, pData, static_cast<DWORD>(Length), NULL, &olp) == FALSE)
			{
				auto lastError = GetLastError();
				if (lastError != ERROR_IO_PENDING)
				{
					nat_Throw(natWinException, lastError, "WriteFile failed."_nv);
				}
			}

			DWORD tWrittenBytes;
			if (!GetOverlappedResult(m_hFile, &olp, &tWrittenBytes, TRUE))
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

	m_pMappedFile = natMemoryStream::CreateFromExternMemory(reinterpret_cast<nData>(pFile), GetSize(), m_bReadable, m_bWritable);

	return m_pMappedFile;
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

natMemoryStream::natMemoryStream(ncData pData, nLen Length, nBool bReadable, nBool bWritable, nBool bResizable)
	: m_pData(nullptr), m_Length(0ul), m_CurPos(0u), m_bReadable(bReadable), m_bWritable(bWritable), m_bResizable(bResizable), m_bExtern(false)
{
	try
	{
		m_pData = new nByte[static_cast<nuInt>(Length)]{};
		m_Length = Length;
		if (pData)
		{
#ifdef _MSC_VER
			memcpy_s(m_pData, static_cast<rsize_t>(Length), pData, static_cast<rsize_t>(Length));
#else
			memcpy(m_pData, pData, static_cast<size_t>(Length));
#endif
		}
	}
	catch (std::bad_alloc&)
	{
		nat_Throw(natException, "Failed to allocate memory"_nv);
	}
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
	return m_bResizable;
}

nBool natMemoryStream::CanSeek() const
{
	return true;
}

nBool natMemoryStream::IsEndOfStream() const
{
	return m_CurPos >= m_Length;
}

nLen natMemoryStream::GetSize() const
{
	return m_Length;
}

void natMemoryStream::SetSize(nLen Size)
{
	if (!m_bResizable)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not resizable."_nv);
	}

	if (Size == m_Length)
	{
		return;
	}

	nData tpData;
	tpData = new nByte[static_cast<nuInt>(Size)];
	for (auto i = 0u; i < Size; ++i)
	{
		tpData[i] = i < m_Length ? m_pData[i] : nByte(0u);
	}

	SafeDelArr(m_pData);
	m_pData = tpData;
	m_Length = Size;

	if (m_CurPos > m_Length)
	{
		m_CurPos = m_Length;
	}
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
		if (Offset < 0 || m_Length < static_cast<nLen>(Offset))
			nat_Throw(natErrException, NatErr_OutOfRange, "Out of range."_nv);
		m_CurPos = Offset;
		break;
	case NatSeek::Cur:
		if ((Offset < 0 && m_CurPos < static_cast<nLen>(-Offset)) || m_Length > static_cast<nLen>(m_CurPos + Offset))
			nat_Throw(natErrException, NatErr_OutOfRange, "Out of range."_nv);
		m_CurPos += Offset;
		break;
	case NatSeek::End:
		if (Offset > 0 || m_Length < static_cast<nLen>(-Offset))
			nat_Throw(natErrException, NatErr_OutOfRange, "Out of range."_nv);
		m_CurPos = m_Length + Offset;
		break;
	default:
		nat_Throw(natErrException, NatErr_OutOfRange, "Out of range."_nv);
	}
}

nByte natMemoryStream::ReadByte()
{
	return m_pData[m_CurPos++];
}

nLen natMemoryStream::ReadBytes(nData pData, nLen Length)
{
	nLen tReadBytes = 0ul;

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

	tReadBytes = std::min(Length, m_Length - m_CurPos);
#ifdef _MSC_VER
	memcpy_s(pData + m_CurPos, static_cast<rsize_t>(m_Length - m_CurPos), m_pData, static_cast<rsize_t>(tReadBytes));
#else
	memcpy(pData + m_CurPos, m_pData, static_cast<size_t>(tReadBytes));
#endif
	
	m_CurPos += tReadBytes;

	return tReadBytes;
}

std::future<nLen> natMemoryStream::ReadBytesAsync(nData pData, nLen Length)
{
	return std::async(std::launch::async, [=]()
	{
		natRefScopeGuard<natCriticalSection> guard(m_CriSection);
		return ReadBytes(pData, Length);
	});
}

void natMemoryStream::WriteByte(nByte byte)
{
	if (m_CurPos >= m_Length)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "End of stream."_nv);
	}

	m_pData[m_CurPos++] = byte;
}

nLen natMemoryStream::WriteBytes(ncData pData, nLen Length)
{
	nLen tWriteBytes = 0ul;

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

	tWriteBytes = std::min(Length, m_Length - m_CurPos);
#ifdef _MSC_VER
	memcpy_s(m_pData, static_cast<rsize_t>(tWriteBytes), pData + m_CurPos, static_cast<rsize_t>(tWriteBytes));
#else
	memcpy(m_pData, pData + m_CurPos, static_cast<size_t>(tWriteBytes));
#endif
	m_CurPos += tWriteBytes;

	return tWriteBytes;
}

std::future<nLen> natMemoryStream::WriteBytesAsync(ncData pData, nLen Length)
{
	return std::async(std::launch::async, [=]()
	{
		natRefScopeGuard<natCriticalSection> guard(m_CriSection);
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

natMemoryStream::natMemoryStream()
	: m_pData(nullptr), m_Length(0ul), m_CurPos(0u), m_bReadable(false), m_bWritable(false), m_bResizable(false), m_bExtern(false)
{
}

natMemoryStream::~natMemoryStream()
{
	if (!m_bExtern)
	{
		SafeDelArr(m_pData);
	}
}

natRefPointer<natMemoryStream> natMemoryStream::CreateFromExternMemory(nData pData, nLen Length, nBool bReadable, nBool bWritable)
{
	auto pStream = make_ref<natMemoryStream>();

	pStream->m_pData = pData;
	pStream->m_Length = Length;
	pStream->m_bReadable = bReadable;
	pStream->m_bWritable = bWritable;
	pStream->m_bExtern = true;

	return pStream;
}
