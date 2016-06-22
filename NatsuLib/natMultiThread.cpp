#include "stdafx.h"
#include "natMultiThread.h"
#include "natException.h"

natThread::natThread(nBool Pause)
{
	m_hThread = CreateThread(NULL, NULL, &execute, static_cast<void *>(this), Pause ? CREATE_SUSPENDED : 0, &m_hThreadID);
	if (m_hThread == NULL)
	{
		nat_Throw(natWinException, _T("Create thread failed"));
	}
}

natThread::~natThread()
{
	CloseHandle(m_hThread);
}

HANDLE natThread::GetHandle() const
{
	return m_hThread;
}

nBool natThread::Resume()
{
	return ResumeThread(m_hThread) != DWORD(-1);
}

nBool natThread::Suspend()
{
	return SuspendThread(m_hThread) != DWORD(-1);
}

DWORD natThread::Wait(nuInt WaitTime)
{
	return WaitForSingleObject(m_hThread, WaitTime);
}

nBool natThread::Terminate(nuInt ExitCode)
{
	return TerminateThread(m_hThread, ExitCode) != FALSE;
}

nuInt natThread::GetExitCode() const
{
	DWORD ExitCode = DWORD(-1);
	GetExitCodeThread(m_hThread, &ExitCode);
	return ExitCode;
}

DWORD natThread::execute(void* p)
{
	return static_cast<natThread *>(p)->ThreadJob();
}

natCriticalSection::natCriticalSection()
{
	InitializeCriticalSection(&m_Section);
}

natCriticalSection::~natCriticalSection()
{
	DeleteCriticalSection(&m_Section);
}

void natCriticalSection::Lock()
{
	EnterCriticalSection(&m_Section);
}

nBool natCriticalSection::TryLock()
{
	return TryEnterCriticalSection(&m_Section) == TRUE;
}

void natCriticalSection::UnLock()
{
	LeaveCriticalSection(&m_Section);
}

natEventWrapper::natEventWrapper(nBool AutoReset, nBool InitialState)
{
	m_hEvent = CreateEvent(NULL, !AutoReset, InitialState, NULL);
	if (m_hEvent == NULL)
	{
		throw natWinException(TEXT("natEventWrapper::natEventWrapper"), TEXT("Create event failed"));
	}
}

natEventWrapper::~natEventWrapper()
{
	CloseHandle(m_hEvent);
}

HANDLE natEventWrapper::GetHandle() const
{
	return m_hEvent;
}

nBool natEventWrapper::Set()
{
	return SetEvent(m_hEvent) != FALSE;
}

nBool natEventWrapper::Reset()
{
	return ResetEvent(m_hEvent) != FALSE;
}

nBool natEventWrapper::Pulse() const
{
	return PulseEvent(m_hEvent) != FALSE;
}

nBool natEventWrapper::Wait(nuInt WaitTime) const
{
	return WaitForSingleObject(m_hEvent, WaitTime) != DWORD(-1);
}
