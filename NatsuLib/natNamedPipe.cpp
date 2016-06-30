#include "stdafx.h"
#include "natNamedPipe.h"
#include "natException.h"
#include <algorithm>

using namespace NatsuLib;

natNamedPipeServerStream::natNamedPipeServerStream(ncTStr Pipename, PipeDirection Direction, nuInt MaxInstances, nuInt OutBuffer, nuInt InBuffer, nuInt TimeOut, PipeMode TransmissionMode, PipeMode ReadMode, PipeOptions Options)
	: m_hPipe(nullptr), m_bAsync(Options == PipeOptions::Asynchronous), m_bConnected(false), m_bMessageComplete(false), m_bReadable(false), m_bWritable(false), m_LastErr(NatErr_OK)
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

nLen natNamedPipeServerStream::ReadBytes(nData pData, nLen Length)
{
	DWORD tReadBytes = 0ul;
	if (!m_bReadable || !m_bConnected)
	{
		m_LastErr = NatErr_IllegalState;
		return tReadBytes;
	}

	if (Length == 0ul)
	{
		return tReadBytes;
	}

	if (pData == nullptr)
	{
		m_LastErr = NatErr_InvalidArg;
		return tReadBytes;
	}

	if (ReadFile(m_hPipe, pData, static_cast<DWORD>(Length), &tReadBytes, NULL) == FALSE)
	{
		m_LastErr = NatErr_InternalErr;
		return tReadBytes;
	}

	if (tReadBytes != Length)
	{
		m_LastErr = NatErr_OutOfRange;
	}

	return tReadBytes;
}

nLen natNamedPipeServerStream::WriteBytes(ncData pData, nLen Length)
{
	DWORD tWriteBytes = 0ul;
	if (!m_bWritable || !m_bConnected)
	{
		m_LastErr = NatErr_IllegalState;
		return tWriteBytes;
	}

	if (Length == 0ul)
	{
		return tWriteBytes;
	}

	if (pData == nullptr)
	{
		m_LastErr = NatErr_InvalidArg;
		return tWriteBytes;
	}

	if (WriteFile(m_hPipe, pData, static_cast<DWORD>(Length), &tWriteBytes, NULL) == FALSE)
	{
		m_LastErr = NatErr_InternalErr;
		return tWriteBytes;
	}

	if (tWriteBytes != Length)
	{
		m_LastErr = NatErr_OutOfRange;
	}

	return tWriteBytes;
}

void natNamedPipeServerStream::Flush()
{
	FlushFileBuffers(m_hPipe);
}

void natNamedPipeServerStream::Lock()
{
	m_Section.Lock();
}

nResult natNamedPipeServerStream::TryLock()
{
	return m_Section.TryLock();
}

void natNamedPipeServerStream::Unlock()
{
	m_Section.UnLock();
}

void natNamedPipeServerStream::WaitForConnection()
{
	if (m_bConnected)
	{
		nat_Throw(natException, _T("Already connected."));
	}

	if (IsAsync())
	{
		natEventWrapper event(true, false);
		OVERLAPPED opd = {0};
		opd.hEvent = event.GetHandle();

		if (!ConnectNamedPipe(m_hPipe, &opd))
		{
			DWORD LastErr = GetLastError();
			if (LastErr != ERROR_IO_PENDING)
			{
				nat_Throw(natWinException, _T("Cannot connect to pipe."));
			}
		}

		if (!event.Wait())
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
		nat_Throw(natException, _T("Already connected."));
	}

	if (IsAsync())
	{
		return std::async([this]()
		{
			natEventWrapper event(true, false);
			OVERLAPPED opd = { 0 };
			opd.hEvent = event.GetHandle();

			if (!ConnectNamedPipe(m_hPipe, &opd))
			{
				DWORD LastErr = GetLastError();
				if (LastErr != ERROR_IO_PENDING)
				{
					nat_Throw(natWinException, _T("Cannot connect to pipe."), LastErr);
				}
			}

			event.Wait();
			m_bConnected = true;
		});
	}
	
	return std::async([this]() { ConnectNamedPipe(m_hPipe, NULL); m_bConnected = true; });
}

natNamedPipeClientStream::natNamedPipeClientStream(ncTStr Pipename, nBool bReadable, nBool bWritable)
	: m_LastErr(NatErr_OK), m_PipeName(Pipename), m_bReadable(bReadable), m_bWritable(bWritable)
{
}

NatErr natNamedPipeClientStream::GetLastErr() const
{
	return m_LastErr == NatErr_OK ? (m_InternalStream ? m_InternalStream->GetLastErr() : NatErr_IllegalState) : m_LastErr;
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

nLen natNamedPipeClientStream::ReadBytes(nData pData, nLen Length)
{
	if (!m_InternalStream)
	{
		m_LastErr = NatErr_IllegalState;
		return 0ul;
	}

	auto ret = m_InternalStream->ReadBytes(pData, Length);
	m_LastErr = NatErr_OK;
	return ret;
}

nLen natNamedPipeClientStream::WriteBytes(ncData pData, nLen Length)
{
	if (!m_InternalStream)
	{
		m_LastErr = NatErr_IllegalState;
		return 0ul;
	}

	auto ret = m_InternalStream->WriteBytes(pData, Length);
	m_LastErr = NatErr_OK;
	return ret;
}

void natNamedPipeClientStream::Flush()
{
	m_InternalStream->Flush();
}

void natNamedPipeClientStream::Lock()
{
	m_InternalStream->Lock();
}

nResult natNamedPipeClientStream::TryLock()
{
	return m_InternalStream->TryLock();
}

void natNamedPipeClientStream::Unlock()
{
	m_InternalStream->Unlock();
}

nResult natNamedPipeClientStream::Wait(nuInt timeOut)
{
	if (m_InternalStream)
	{
		return m_LastErr = NatErr_IllegalState;
	}

	if (!WaitNamedPipe(m_PipeName.c_str(), timeOut == Infinity ? NMPWAIT_WAIT_FOREVER : timeOut))
	{
		return m_LastErr = NatErr_InternalErr;
	}

	m_InternalStream = move(std::make_unique<natFileStream>(m_PipeName.c_str(), m_bReadable, m_bWritable));

	return m_LastErr = NatErr_OK;
}
