#include "stdafx.h"
#include "natNamedPipe.h"
#include "natException.h"
#include <algorithm>

using namespace NatsuLib;

#ifdef _WIN32

natNamedPipeServerStream::natNamedPipeServerStream(ncTStr Pipename, PipeDirection Direction, nuInt MaxInstances, nuInt OutBuffer, nuInt InBuffer, nuInt TimeOut, PipeMode TransmissionMode, PipeMode ReadMode, PipeOptions Options)
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
		nat_Throw(natException, _T("Unknown Direction."));
	}

	m_hPipe = CreateNamedPipe(
		Pipename,
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
		nat_Throw(natWinException, _T("Cannot create pipe."));
	}
}

natNamedPipeServerStream::~natNamedPipeServerStream()
{
	DisconnectNamedPipe(m_hPipe);
	CloseHandle(m_hPipe);
}

nByte natNamedPipeServerStream::ReadByte()
{
	nByte byte;
	if (ReadBytes(&byte, 1) == 1)
	{
		return byte;
	}

	nat_Throw(natErrException, NatErr_InternalErr, _T("Unable to read byte."));
}

nLen natNamedPipeServerStream::ReadBytes(nData pData, nLen Length)
{
	DWORD tReadBytes = 0ul;
	if (!m_bReadable || !m_bConnected)
	{
		nat_Throw(natErrException, NatErr_IllegalState, _T("Stream is not readable or not connected."));
	}

	if (Length == 0ul)
	{
		return tReadBytes;
	}

	if (pData == nullptr)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, _T("pData cannot be nullptr"));
	}

	if (ReadFile(m_hPipe, pData, static_cast<DWORD>(Length), &tReadBytes, NULL) == FALSE)
	{
		nat_Throw(natWinException, _T("ReadFile failed."));
	}

	return tReadBytes;
}

std::future<nLen> natNamedPipeServerStream::ReadBytesAsync(nData pData, nLen Length)
{
	return std::async([=]()
	{
		return ReadBytes(pData, Length);
	});
}

void natNamedPipeServerStream::WriteByte(nByte byte)
{
	if (WriteBytes(&byte, 1) != 1)
	{
		nat_Throw(natErrException, NatErr_InternalErr, _T("Unable to write byte."));
	}
}

nLen natNamedPipeServerStream::WriteBytes(ncData pData, nLen Length)
{
	DWORD tWriteBytes = 0ul;
	if (!m_bWritable || !m_bConnected)
	{
		nat_Throw(natErrException, NatErr_IllegalState, _T("Stream is not writable or not connected."));
	}

	if (Length == 0ul)
	{
		return tWriteBytes;
	}

	if (pData == nullptr)
	{
		nat_Throw(natErrException, NatErr_InvalidArg, _T("pData cannot be nullptr"));
	}

	if (WriteFile(m_hPipe, pData, static_cast<DWORD>(Length), &tWriteBytes, NULL) == FALSE)
	{
		nat_Throw(natWinException, _T("WriteFile failed."));
	}

	return tWriteBytes;
}

std::future<nLen> natNamedPipeServerStream::WriteBytesAsync(ncData pData, nLen Length)
{
	return std::async([=]()
	{
		return WriteBytes(pData, Length);
	});
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
			nat_Throw(natWinException, _T("CreateEvent failed."));
		}
		OVERLAPPED opd = { 0 };
		opd.hEvent = event;

		if (!ConnectNamedPipe(m_hPipe, &opd))
		{
			DWORD LastErr = GetLastError();
			if (LastErr != ERROR_IO_PENDING)
			{
				nat_Throw(natWinException, _T("Cannot connect to pipe."));
			}
		}

		if (!WaitForSingleObject(event, INFINITE))
		{
			nat_Throw(natWinException, _T("Cannot connect to pipe."));
		}
	}
	else
	{
		if (!ConnectNamedPipe(m_hPipe, nullptr))
		{
			nat_Throw(natWinException, _T("Cannot connect to pipe."));
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
				nat_Throw(natWinException, _T("CreateEvent failed."));
			}
			OVERLAPPED opd = { 0 };
			opd.hEvent = event;

			if (!ConnectNamedPipe(m_hPipe, &opd))
			{
				DWORD LastErr = GetLastError();
				if (LastErr != ERROR_IO_PENDING)
				{
					nat_Throw(natWinException, _T("Cannot connect to pipe."));
				}
			}

			WaitForSingleObject(event, INFINITE);
			m_bConnected = true;
		});
	}

	return std::async([this]() { ConnectNamedPipe(m_hPipe, NULL); m_bConnected = true; });
}

natNamedPipeClientStream::natNamedPipeClientStream(ncTStr Pipename, nBool bReadable, nBool bWritable)
	: m_PipeName(Pipename), m_bReadable(bReadable), m_bWritable(bWritable)
{
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
	nByte byte;
	if (ReadBytes(&byte, 1) == 1)
	{
		return byte;
	}

	nat_Throw(natErrException, NatErr_InternalErr, _T("Unable to read byte."));
}

nLen natNamedPipeClientStream::ReadBytes(nData pData, nLen Length)
{
	if (!m_InternalStream)
	{
		nat_Throw(natErrException, NatErr_IllegalState, _T("Internal stream is not ready."));
	}

	auto ret = m_InternalStream->ReadBytes(pData, Length);
	return ret;
}

std::future<nLen> natNamedPipeClientStream::ReadBytesAsync(nData pData, nLen Length)
{
	if (!m_InternalStream)
	{
		nat_Throw(natErrException, NatErr_IllegalState, _T("Internal stream is not ready."));
	}

	auto ret = m_InternalStream->ReadBytesAsync(pData, Length);
	return ret;
}

void natNamedPipeClientStream::WriteByte(nByte byte)
{
	if (WriteBytes(&byte, 1) != 1)
	{
		nat_Throw(natErrException, NatErr_InternalErr, _T("Unable to write byte."));
	}
}

nLen natNamedPipeClientStream::WriteBytes(ncData pData, nLen Length)
{
	if (!m_InternalStream)
	{
		nat_Throw(natErrException, NatErr_IllegalState, _T("Internal stream is not ready."));
	}

	auto ret = m_InternalStream->WriteBytes(pData, Length);
	return ret;
}

std::future<nLen> natNamedPipeClientStream::WriteBytesAsync(ncData pData, nLen Length)
{
	if (!m_InternalStream)
	{
		nat_Throw(natErrException, NatErr_IllegalState, _T("Internal stream is not ready."));
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
		nat_Throw(natErrException, NatErr_IllegalState, _T("Internal stream is not ready."));
	}

	if (!WaitNamedPipe(m_PipeName.c_str(), timeOut == Infinity ? NMPWAIT_WAIT_FOREVER : timeOut))
	{
		nat_Throw(natWinException, _T("WaitNamedPipe failed."));
	}

	m_InternalStream = std::make_unique<natFileStream>(m_PipeName.c_str(), m_bReadable, m_bWritable, true);
}

#endif
