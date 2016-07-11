////////////////////////////////////////////////////////////////////////////////
///	@file	natMultiThread.h
///	@brief	多线程支持
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <Windows.h>
#include <unordered_map>
#include <memory>
#include <functional>
#include <atomic>
#include <queue>
#include <future>

#pragma push_macro("max")
#ifdef max
#	undef max
#endif

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@addtogroup	系统底层支持
	///	@brief		提供涉及系统内核操作的支持
	///	@{

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	线程基类
	///	@note	需继承该类并重写ThreadJob以实现多线程\n
	///			请确保线程类在执行期间保持有效
	////////////////////////////////////////////////////////////////////////////////
	class natThread
	{
	public:
		typedef unsigned long ThreadIdType;
		typedef unsigned long ResultType;
		typedef nUnsafePtr<void> UnsafeHandle;

		enum : nuInt
		{
			Infinity = std::numeric_limits<nuInt>::max(),
		};

		///	@brief	构造函数
		///	@param[in]	Pause	创建线程时暂停
		explicit natThread(nBool Pause = true);
		virtual ~natThread();

		///	@brief		获得线程句柄
		///	@warning	请勿手动删除
		UnsafeHandle GetHandle() const noexcept;

		ThreadIdType GetThreadId() const noexcept;

		///	@brief	继续线程执行
		///	@return	是否成功
		nBool Resume() noexcept;

		///	@brief	阻塞线程执行
		///	@return	是否成功
		nBool Suspend() noexcept;

		///	@brief	等待线程执行
		///	@param[in]	WaitTime	等待时间
		///	@return	等待线程状态
		ResultType Wait(nuInt WaitTime = Infinity) noexcept;

		///	@brief	结束线程
		///	@param[in]	ExitCode	退出码
		///	@return	是否成功
		nBool Terminate(nuInt ExitCode = nuInt(-1)) noexcept;

		///	@brief	获得退出码
		nuInt GetExitCode() const;
	protected:
		///	@brief	重写此方法以实现线程工作
		virtual ResultType ThreadJob() = 0;

	private:
		///	@brief	执行线程的包装函数
		///	@param[in]	p	指向Thread类的指针
		static ResultType CALLBACK execute(void* p);
		UnsafeHandle m_hThread;
		ThreadIdType m_hThreadID;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	临界区
	///	@note	用于快速为代码加锁以解决多线程冲突
	////////////////////////////////////////////////////////////////////////////////
	class natCriticalSection final
	{
	public:
		natCriticalSection();
		~natCriticalSection();

		///	@brief	锁定临界区
		///	@note	可能引发死锁
		void Lock();
		///	@brief	尝试锁定临界区
		///	@note	不会阻塞线程
		///	@return	是否成功
		nBool TryLock();
		///	@brief	解锁临界区
		void UnLock();
	private:
		CRITICAL_SECTION m_Section;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	Windows的Event包装类
	///	@note	通过Event对多线程操作进行同步
	////////////////////////////////////////////////////////////////////////////////
	class natEventWrapper final
	{
	public:
		typedef nUnsafePtr<void> UnsafeHandle;
		enum : nuInt
		{
			Infinity = std::numeric_limits<nuInt>::max(),
		};

		natEventWrapper(nBool AutoReset, nBool InitialState);
		~natEventWrapper();

		///	@brief		获得句柄
		///	@warning	请勿手动释放
		UnsafeHandle GetHandle() const;

		///	@brief		标记事件
		///	@return		是否成功
		nBool Set();

		///	@brief		取消事件标记
		///	@return		是否成功
		nBool Reset();

		///	@brief		事件脉冲
		///	@return		是否成功
		nBool Pulse() const;

		///	@brief		等待事件
		///	@param[in]	WaitTime	等待时间
		///	@return		是否成功
		nBool Wait(nuInt WaitTime = Infinity) const;
	private:
		UnsafeHandle m_hEvent;
	};

	class natThreadPool final
	{
		class WorkToken;
	public:
		typedef std::function<nuInt(void*)> WorkFunc;
		typedef unsigned long ThreadIdType;
		typedef natThread::ResultType ResultType;
		enum : nuInt
		{
			DefaultMaxThreadCount = 4,
			Infinity = std::numeric_limits<nuInt>::max(),
		};

		explicit natThreadPool(nuInt InitialThreadCount = 0, nuInt MaxThreadCount = DefaultMaxThreadCount);
		~natThreadPool();

		void KillIdleThreads(nBool Force = false);
		void KillAllThreads(nBool Force = false);
		std::future<WorkToken> QueueWork(WorkFunc workFunc, void* param = nullptr);
		ThreadIdType GetThreadId(nuInt Index) const;
		void WaitAllJobsFinish(nuInt WaitTime = Infinity);

	private:
		class WorkerThread final
			: public natThread
		{
		public:
			WorkerThread(natThreadPool* pPool, nuInt Index);
			WorkerThread(natThreadPool* pPool, nuInt Index, WorkFunc CallableObj, void* Param = nullptr);
			~WorkerThread() = default;

			WorkerThread(WorkerThread const&) = delete;
			WorkerThread& operator=(WorkerThread const&) = delete;

			nBool IsIdle() const;
			std::future<nuInt> SetWork(WorkFunc CallableObj, void* Param = nullptr);

			void RequestTerminate(nBool value = true);

		private:
			ResultType ThreadJob() override;

			natThreadPool* m_pPool;
			const nuInt m_Index;
			WorkFunc m_CallableObj;
			void* m_Arg;
			std::promise<nuInt> m_LastResult;

			std::atomic<nBool> m_Idle, m_ShouldTerminate;
		};

		class WorkToken final
		{
		public:
			WorkToken() = default;
			WorkToken(nuInt workThreadId, std::future<nuInt>&& result)
				: m_WorkThreadIndex(workThreadId), m_Result(move(result))
			{
			}
			WorkToken(WorkToken&& other)
				: m_WorkThreadIndex(other.m_WorkThreadIndex), m_Result(move(other.m_Result))
			{
			}
			~WorkToken() = default;

			WorkToken& operator=(WorkToken&& other)
			{
				m_WorkThreadIndex = other.m_WorkThreadIndex;
				m_Result = move(other.m_Result);
				return *this;
			}

			nuInt GetWorkThreadIndex() const noexcept
			{
				return m_WorkThreadIndex;
			}

			std::future<nuInt>& GetResult() noexcept
			{
				return m_Result;
			}

		private:
			nuInt m_WorkThreadIndex;
			std::future<nuInt> m_Result;
		};

		nuInt getNextAvailableIndex();
		nuInt getIdleThreadIndex();
		void onWorkerThreadIdle(nuInt Index);

		const nuInt m_MaxThreadCount;
		std::unordered_map<nuInt, std::unique_ptr<WorkerThread>> m_Threads;
		std::queue<std::tuple<WorkFunc, void*, std::promise<WorkToken>>> m_WorkQueue;
		natCriticalSection m_Section;
	};

	///	@}
}

#pragma pop_macro("max")
