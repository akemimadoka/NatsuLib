#include "stdafx.h"
#include "natMultiThread.h"
#include "natException.h"
#include "natMisc.h"

#ifdef max
#	undef max
#endif

using namespace NatsuLib;

#ifdef WIN32
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

natThread::UnsafeHandle natThread::GetHandle() noexcept
{
	return m_hThread;
}

natThread::ThreadIdType natThread::GetThreadId() const noexcept
{
	return m_hThreadID;
}

nBool natThread::Resume() noexcept
{
	return ResumeThread(m_hThread) != DWORD(-1);
}

nBool natThread::Suspend() noexcept
{
	return SuspendThread(m_hThread) != DWORD(-1);
}

nBool natThread::Wait(nuInt WaitTime) noexcept
{
	return WaitForSingleObject(m_hThread, WaitTime);
}

nBool natThread::Terminate(nuInt ExitCode) noexcept
{
	return TerminateThread(m_hThread, ExitCode) != FALSE;
}

nuInt natThread::GetExitCode()
{
	DWORD ExitCode = DWORD(-1);
	if (!GetExitCodeThread(m_hThread, &ExitCode))
	{
		nat_Throw(natWinException, _T("GetExitCodeThread failed."));
	}
	return ExitCode;
}

natThread::ResultType natThread::execute(void* p)
{
	return static_cast<natThread *>(p)->ThreadJob();
}
#else
natThread::natThread(nBool Pause)
	: m_Thread([this]()
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	m_Pause.wait(lock);
	std::promise<ResultType> Result;
	m_Result = move(Result.get_future());
	Result.set_value_at_thread_exit(ThreadJob());
})
{
	m_Thread.join();
	if (!Pause)
	{
		m_Pause.notify_all();
	}
}

natThread::~natThread()
{
}

natThread::UnsafeHandle natThread::GetHandle() noexcept
{
	return m_Thread.native_handle();
}

natThread::ThreadIdType natThread::GetThreadId() const noexcept
{
	return m_Thread.get_id();
}

nBool natThread::Resume() noexcept
{
	m_Pause.notify_all();
	return true;
}

nBool natThread::Suspend() noexcept
{
	return false;
}

nBool natThread::Wait(nuInt WaitTime) noexcept
{
	return m_Result.wait_for(std::chrono::milliseconds(WaitTime)) != std::future_status::timeout;
}

nBool natThread::Terminate(nuInt ExitCode) noexcept
{
	return false;
}

nuInt natThread::GetExitCode()
{
	return m_Result.get();
}
#endif

#ifdef WIN32
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
#else
natCriticalSection::natCriticalSection()
{
}

natCriticalSection::~natCriticalSection()
{
}

void natCriticalSection::Lock()
{
	m_Mutex.lock();
}

nBool natCriticalSection::TryLock()
{
	return m_Mutex.try_lock();
}

void natCriticalSection::UnLock()
{
	m_Mutex.unlock();
}

#endif

#ifdef WIN32
natEventWrapper::natEventWrapper(nBool AutoReset, nBool InitialState)
{
	m_hEvent = CreateEvent(NULL, !AutoReset, InitialState, NULL);
	if (m_hEvent == NULL)
	{
		nat_Throw(natWinException, _T("Create event failed"));
	}
}

natEventWrapper::~natEventWrapper()
{
	CloseHandle(m_hEvent);
}

natEventWrapper::UnsafeHandle natEventWrapper::GetHandle()
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

nBool natEventWrapper::Pulse()
{
	return PulseEvent(m_hEvent) != FALSE;
}

nBool natEventWrapper::Wait(nuInt WaitTime)
{
	return WaitForSingleObject(m_hEvent, WaitTime) != DWORD(-1);
}
#else
natEventWrapper::natEventWrapper(nBool AutoReset, nBool InitialState)
{
}

natEventWrapper::~natEventWrapper()
{
}

natEventWrapper::UnsafeHandle natEventWrapper::GetHandle()
{
	return m_Condition.native_handle();
}

nBool natEventWrapper::Set()
{
	m_Condition.notify_all();
	return true;
}

nBool natEventWrapper::Reset()
{
	return false;
}

nBool natEventWrapper::Pulse()
{
	m_Condition.notify_all();
	return true;
}

nBool natEventWrapper::Wait(nuInt WaitTime)
{
	std::unique_lock<std::mutex> m_lock(m_Mutex);
	return m_Condition.wait_for(m_lock, std::chrono::milliseconds(WaitTime)) != std::cv_status::timeout;
}
#endif

natThreadPool::natThreadPool(nuInt InitialThreadCount, nuInt MaxThreadCount)
	: m_MaxThreadCount(MaxThreadCount)
{
	if (m_MaxThreadCount < InitialThreadCount)
	{
		nat_Throw(natException, _T("Max thread count({0}) should be bigger than total thread count({1})."), m_MaxThreadCount, InitialThreadCount);
	}

	nuInt Index;
	while (InitialThreadCount)
	{
		--InitialThreadCount;
		Index = getNextAvailableIndex();
		m_Threads[Index] = move(std::make_unique<WorkerThread>(this, Index));
	}
}

natThreadPool::~natThreadPool()
{
}

void natThreadPool::KillIdleThreads(nBool Force)
{
	for (auto&& thread : m_Threads)
	{
		if (thread.second->IsIdle())
		{
			if (Force)
			{
				if (!thread.second->Terminate(0))
				{
					nat_Throw(natWinException, _T("Cannot terminate thread."));
				}
			}
			else
			{
				thread.second->RequestTerminate();
			}
		}
	}
}

void natThreadPool::KillAllThreads(nBool Force)
{
	for (auto&& thread : m_Threads)
	{
		if (Force)
		{
			if (!thread.second->Terminate(0))
			{
				nat_Throw(natWinException, _T("Cannot terminate thread."));
			}
		}
		else
		{
			thread.second->RequestTerminate();
		}
	}
}

std::future<natThreadPool::WorkToken> natThreadPool::QueueWork(WorkFunc workFunc, void* param)
{
	auto Index = getIdleThreadIndex();
	if (Index == std::numeric_limits<nuInt>::max() && m_Threads.size() < m_MaxThreadCount)
	{
		auto availableIndex = getNextAvailableIndex();
		auto&& thread = m_Threads[availableIndex] = move(std::make_unique<WorkerThread>(this, availableIndex));
		auto&& result = thread->SetWork(workFunc, param);
		std::promise<WorkToken> dummyPromise;
		auto ret = dummyPromise.get_future();
		dummyPromise.set_value(WorkToken(availableIndex, move(result)));
		return move(ret);
	}

	if (Index != std::numeric_limits<nuInt>::max())
	{
		auto&& result = m_Threads[Index]->SetWork(workFunc, param);
		std::promise<WorkToken> dummyPromise;
		auto ret = dummyPromise.get_future();
		dummyPromise.set_value(WorkToken(Index, move(result)));
		return move(ret);
	}

	auto work = make_tuple(workFunc, param, std::promise<WorkToken>());
	auto ret = std::get<2>(work).get_future();
	m_WorkQueue.push(move(work));
	return move(ret);
}

natThread::ThreadIdType natThreadPool::GetThreadId(nuInt Index) const
{
	auto iter = m_Threads.find(Index);
	if (iter == m_Threads.end())
	{
		nat_Throw(natException, _T("No such thread with index {0}."), Index);
	}

	return iter->second->GetThreadId();
}

void natThreadPool::WaitAllJobsFinish(nuInt WaitTime)
{
	KillIdleThreads();
	for (auto&& thread : m_Threads)
	{
		thread.second->RequestTerminate();
		thread.second->Wait(WaitTime);
	}
}

natThreadPool::WorkerThread::WorkerThread(natThreadPool* pPool, nuInt Index)
	: natThread(true), m_pPool(pPool), m_Index(Index), m_Arg(nullptr), m_Idle(true), m_ShouldTerminate(false)
{
}

natThreadPool::WorkerThread::WorkerThread(natThreadPool* pPool, nuInt Index,WorkFunc CallableObj, void* Param)
	: WorkerThread(pPool, Index)
{
	SetWork(CallableObj, Param);
}

nBool natThreadPool::WorkerThread::IsIdle() const
{
	return m_Idle;
}

std::future<nuInt> natThreadPool::WorkerThread::SetWork(WorkFunc CallableObj, void* Param)
{
	m_CallableObj = CallableObj;
	m_Arg = Param;
	m_Idle = false;
	m_LastResult = std::promise<nuInt>();
	if (!Resume())
	{
		nat_Throw(natWinException, _T("Cannot resume worker thread."));
	}
	return move(m_LastResult.get_future());
}

void natThreadPool::WorkerThread::RequestTerminate(nBool value)
{
	m_ShouldTerminate = value;
	if (m_Idle)
	{
		if (!Resume())
		{
			if (!Terminate(0))
			{
				nat_Throw(natWinException, _T("Cannot terminate worker thread."));
			}
		}
	}
}

natThread::ResultType natThreadPool::WorkerThread::ThreadJob()
{
	while (!m_ShouldTerminate)
	{
		m_Idle = false;
		m_LastResult.set_value(m_CallableObj(m_Arg));
		m_Idle = true;
		m_pPool->onWorkerThreadIdle(m_Index);
		if (!m_ShouldTerminate && m_Idle && !Suspend())
		{
			nat_Throw(natWinException, _T("Cannot suspend worker thread."));
		}
	}
	
	return NatErr_OK;
}

nuInt natThreadPool::getNextAvailableIndex()
{
	m_Section.Lock();
	auto scope = std::move(make_scope([this]()
	{
		m_Section.UnLock();
	}));

	for (nuInt i = 0; i < std::numeric_limits<nuInt>::max(); ++i)
	{
		if (m_Threads.find(i) == m_Threads.end())
		{
			return i;
		}
	}

	nat_Throw(natException, _T("No available index."));
}

nuInt natThreadPool::getIdleThreadIndex()
{
	m_Section.Lock();
	auto scope = std::move(make_scope([this]()
	{
		m_Section.UnLock();
	}));

	for (auto&& thread : m_Threads)
	{
		if (thread.second->IsIdle())
		{
			return thread.first;
		}
	}

	return std::numeric_limits<nuInt>::max();
}

void natThreadPool::onWorkerThreadIdle(nuInt Index)
{
	m_Section.Lock();
	auto scope = std::move(make_scope([this]()
	{
		m_Section.UnLock();
	}));

	if (!m_WorkQueue.empty())
	{
		auto&& work = m_WorkQueue.front();
		auto&& ret = m_Threads[Index]->SetWork(std::get<0>(work), std::get<1>(work));
		std::get<2>(work).set_value(WorkToken(Index, move(ret)));
		m_WorkQueue.pop();
	}
}
