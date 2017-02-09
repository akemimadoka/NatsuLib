#include "stdafx.h"
#include "natNamedPipe.h"
#include "natException.h"
#include <algorithm>

using namespace NatsuLib;

#ifdef _WIN32

natNamedPipeServerStream::natNamedPipeServerStream(nStrView Pipename, PipeDirection Direction, nuInt MaxInstances, nuInt OutBuffer, nuInt InBuffer, nuInt TimeOut, PipeMode TransmissionMode, PipeMode ReadMode, PipeOptions Options)
	: m_hPipe(nullptr), m_bAsync(Options == PipeOptions::Asynchronous), m_bConnected(false), m_bMessageComplete(false), m_bReadable(false), m_bWritable(false)
{
	DWORD dir;
	switch (Direction)
	{
	case PipeDirection::In:
		dir = PIPE_ACCESS_INBOUND;
		m_bReadable = true;
		break;
	case PipeDirection::Out:
		dir = PIPE_ACCESS_OUTBOUND;
		m_bWritable = true;
		break;
	case PipeDirection::Inout:
		dir = PIPE_ACCESS_DUPLEX;
		m_bReadable = m_bWritable = true;
		break;
	default:
		nat_Throw(natException, "Unknown Direction."_nv);
	}

	m_hPipe = CreateNamedPipe(
#ifdef UNICODE
		WideString{ Pipename }.data()
#else
		AnsiString{ Pipename }.data()
#endif
		,
		dir | (m_bAsync ? FILE_FLAG_OVERLAPPED : FILE_FLAG_WRITE_THROUGH),
		(TransmissionMode == PipeMode::Byte ? PIPE_TYPE_BYTE : PIPE_TYPE_MESSAGE) |
		(m_bReadable ? (ReadMode == PipeMode::Byte ? PIPE_READMODE_BYTE : PIPE_READMODE_MESSAGE) : 0) |
		(m_bAsync ? PIPE_NOWAIT : PIPE_WAIT),
		std::min(nuInt(PIPE_UNLIMITED_INSTANCES), MaxInstances),
		OutBuffer,
		InBuffer,
		TimeOut,
		NULL);

	if (m_hPipe == INVALID_HANDLE_VALUE || !m_hPipe)
	{
		nat_Throw(natWinException, "Cannot create pipe."_nv);
	}
}

natNamedPipeServerStream::~natNamedPipeServerStream()
{
	DisconnectNamedPipe(m_hPipe);
	CloseHandle(m_hPipe);
}

nBool natNamedPipeServerStream::CanWrite() const
{
	return m_bWritable;
}

nBool natNamedPipeServerStream::CanRead() const
{
	return m_bReadable;
}

nBool natNamedPipeServerStream::CanResize() const
{
	return false;
}

nBool natNamedPipeServerStream::CanSeek() const
{
	return false;
}

nBool natNamedPipeServerStream::IsEndOfStream() const
{
	return false;
}

nLen natNamedPipeServerStream::GetSize() const
{
	return 0ul;
}

void natNamedPipeServerStream::SetSize(nLen)
{
	nat_Throw(natErrException, NatErr_NotSupport, "This type of stream does not support SetSize."_nv);
}

nLen natNamedPipeServerStream::GetPosition() const
{
	return 0ul;
}

void natNamedPipeServerStream::SetPosition(NatSeek, nLong)
{
	nat_Throw(natErrException, NatErr_NotSupport, "This type of stream does not support SetPosition."_nv);
}

nLen natNamedPipeServerStream::ReadBytes(nData pData, nLen Length)
{
	DWORD tReadBytes = 0ul;
	if (!m_bReadable || !m_bConnected)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not readable or not connected."_nv);
	}

	if (Length == 0ul)
	{
		return tReadBytes;
	}

	if (pData == nullptr)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "pData cannot be nullptr"_nv);
	}

	if (ReadFile(m_hPipe, pData, static_cast<DWORD>(Length), &tReadBytes, NULL) == FALSE)
	{
		nat_Throw(natWinException, "ReadFile failed."_nv);
	}

	return tReadBytes;
}

nLen natNamedPipeServerStream::WriteBytes(ncData pData, nLen Length)
{
	DWORD tWriteBytes = 0ul;
	if (!m_bWritable || !m_bConnected)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Stream is not writable or not connected."_nv);
	}

	if (Length == 0ul)
	{
		return tWriteBytes;
	}

	if (pData == nullptr)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "pData cannot be nullptr"_nv);
	}

	if (WriteFile(m_hPipe, pData, static_cast<DWORD>(Length), &tWriteBytes, NULL) == FALSE)
	{
		nat_Throw(natWinException, "WriteFile failed."_nv);
	}

	return tWriteBytes;
}

void natNamedPipeServerStream::Flush()
{
	FlushFileBuffers(m_hPipe);
}

void natNamedPipeServerStream::WaitForConnection()
{
	if (m_bConnected)
	{
		return;
	}

	if (IsAsync())
	{
		auto event = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!event || event == INVALID_HANDLE_VALUE)
		{
			nat_Throw(natWinException, "CreateEvent failed."_nv);
		}
		OVERLAPPED opd = { 0 };
		opd.hEvent = event;

		if (!ConnectNamedPipe(m_hPipe, &opd))
		{
			DWORD LastErr = GetLastError();
			if (LastErr != ERROR_IO_PENDING)
			{
				nat_Throw(natWinException, "Cannot connect to pipe."_nv);
			}
		}

		if (!WaitForSingleObject(event, INFINITE))
		{
			nat_Throw(natWinException, "Cannot connect to pipe."_nv);
		}
	}
	else
	{
		if (!ConnectNamedPipe(m_hPipe, nullptr))
		{
			nat_Throw(natWinException, "Cannot connect to pipe."_nv);
		}
	}

	m_bConnected = true;
}

std::future<void> natNamedPipeServerStream::WaitForConnectionAsync()
{
	if (m_bConnected)
	{
		std::promise<void> dummy;
		dummy.set_value();
		return dummy.get_future();
	}

	if (IsAsync())
	{
		return std::async([this]()
		{
			auto event = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (!event || event == INVALID_HANDLE_VALUE)
			{
				nat_Throw(natWinException, "CreateEvent failed."_nv);
			}
			OVERLAPPED opd = { 0 };
			opd.hEvent = event;

			if (!ConnectNamedPipe(m_hPipe, &opd))
			{
				DWORD LastErr = GetLastError();
				if (LastErr != ERROR_IO_PENDING)
				{
					nat_Throw(natWinException, "Cannot connect to pipe."_nv);
				}
			}

			WaitForSingleObject(event, INFINITE);
			m_bConnected = true;
		});
	}

	return std::async([this]() { ConnectNamedPipe(m_hPipe, NULL); m_bConnected = true; });
}

natNamedPipeClientStream::natNamedPipeClientStream(nStrView Pipename, nBool bReadable, nBool bWritable)
	: m_PipeName(Pipename), m_bReadable(bReadable), m_bWritable(bWritable)
{
}

natNamedPipeClientStream::~natNamedPipeClientStream()
{
}

nBool natNamedPipeClientStream::CanResize() const
{
	return false;
}

nBool natNamedPipeClientStream::CanSeek() const
{
	return false;
}

nLen natNamedPipeClientStream::GetSize() const
{
	return 0ul;
}

nBool natNamedPipeClientStream::IsEndOfStream() const
{
	return false;
}

void natNamedPipeClientStream::SetSize(nLen)
{
	nat_Throw(natErrException, NatErr_NotSupport, "This type of stream does not support SetSize."_nv);
}

nLen natNamedPipeClientStream::GetPosition() const
{
	return 0ul;
}

void natNamedPipeClientStream::SetPosition(NatSeek, nLong)
{
	nat_Throw(natErrException, NatErr_NotSupport, "This type of stream does not support SetPosition."_nv);
}

nBool natNamedPipeClientStream::CanWrite() const
{
	if (!m_InternalStream)
	{
		return false;
	}

	return m_InternalStream->CanWrite();
}

nBool natNamedPipeClientStream::CanRead() const
{
	if (!m_InternalStream)
	{
		return false;
	}

	return m_InternalStream->CanRead();
}

nByte natNamedPipeClientStream::ReadByte()
{
	return m_InternalStream->ReadByte();
}

nLen natNamedPipeClientStream::ReadBytes(nData pData, nLen Length)
{
	if (!m_InternalStream)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Internal stream is not ready."_nv);
	}

	auto ret = m_InternalStream->ReadBytes(pData, Length);
	return ret;
}

std::future<nLen> natNamedPipeClientStream::ReadBytesAsync(nData pData, nLen Length)
{
	if (!m_InternalStream)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Internal stream is not ready."_nv);
	}

	auto ret = m_InternalStream->ReadBytesAsync(pData, Length);
	return ret;
}

void natNamedPipeClientStream::WriteByte(nByte byte)
{
	m_InternalStream->WriteByte(byte);
}

nLen natNamedPipeClientStream::WriteBytes(ncData pData, nLen Length)
{
	if (!m_InternalStream)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Internal stream is not ready."_nv);
	}

	auto ret = m_InternalStream->WriteBytes(pData, Length);
	return ret;
}

std::future<nLen> natNamedPipeClientStream::WriteBytesAsync(ncData pData, nLen Length)
{
	if (!m_InternalStream)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Internal stream is not ready."_nv);
	}

	auto ret = m_InternalStream->WriteBytesAsync(pData, Length);
	return ret;
}

void natNamedPipeClientStream::Flush()
{
	m_InternalStream->Flush();
}

void natNamedPipeClientStream::Wait(nuInt timeOut)
{
	if (m_InternalStream)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Internal stream is not ready."_nv);
	}

	if (!WaitNamedPipe(
#ifdef UNICODE
		WideString{ m_PipeName }.data()
#else
		AnsiString{ m_PipeName }.data()
#endif
			, timeOut == Infinity ? NMPWAIT_WAIT_FOREVER : timeOut))
	{
		nat_Throw(natWinException, "WaitNamedPipe failed."_nv);
	}

	m_InternalStream = make_ref<natFileStream>(m_PipeName.GetView(), m_bReadable, m_bWritable);
}

#endif
