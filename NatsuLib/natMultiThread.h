////////////////////////////////////////////////////////////////////////////////
///	@file	natMultiThread.h
///	@brief	多线程支持
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natConfig.h"
#include "natDelegate.h"
#ifdef _WIN32
#	include <Windows.h>
#endif
#include <unordered_map>
#include <memory>
#include <atomic>
#include <queue>
#include <future>
#include "natMisc.h"

#ifdef _MSC_VER
#	pragma push_macro("max")
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
		: public noncopyable
	{
	public:
		typedef std::thread::id ThreadIdType;
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
		UnsafeHandle GetHandle() noexcept;

		ThreadIdType GetThreadId() const noexcept;

		///	@brief	继续线程执行
		///	@return	是否成功
		void Resume() noexcept;

		///	@brief	等待线程执行
		///	@param[in]	WaitTime	等待时间
		///	@return	返回时是否未超时
		nBool Wait(nuInt WaitTime = Infinity) const noexcept;

		///	@brief	获得退出码
		nuInt GetExitCode();

	protected:
		///	@brief	重写此方法以实现线程工作
		virtual ResultType ThreadJob() = 0;

	private:
		std::atomic_bool m_Paused;
		std::promise<void> m_Pause;
		std::future<ResultType> m_Result;
		std::thread m_Thread;
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
#ifdef _WIN32
		CRITICAL_SECTION m_Section;
#else
		std::recursive_mutex m_Mutex;
#endif
	};

	namespace detail_
	{
		template <typename T, typename Enable = void>
		struct IsLockableAndUnlockable
			: std::false_type
		{
		};

		template <typename T>
		struct IsLockableAndUnlockable<T, std::void_t<decltype(std::declval<T>().Lock()), decltype(std::declval<T>().UnLock()), decltype(std::declval<T>().TryLock())>>
			: std::true_type
		{
		};
	}

	template <typename T, std::enable_if_t<detail_::IsLockableAndUnlockable<T>::value, bool> = true>
	class natScopeGuard
		: public std::conditional_t<std::disjunction<std::is_move_constructible<T>, std::is_move_assignable<T>>::value, nonmovable, noncopyable>
	{
	public:
		typedef T LockObj;

		template <typename... Args>
		explicit constexpr natScopeGuard(Args&&... args)
			: m_LockObj{ std::forward<Args>(args)... }
		{
			m_LockObj.Lock();
		}

		~natScopeGuard()
		{
			m_LockObj.UnLock();
		}

		decltype(auto) TryLock()
		{
			return m_LockObj.TryLock();
		}

		LockObj& GetObj()
		{
			return m_LockObj;
		}

		LockObj const& GetObj() const
		{
			return m_LockObj;
		}

	private:
		T m_LockObj;
	};

	template <typename... T>
	class natRefScopeGuard
		: public nonmovable
	{
		static_assert(std::conjunction<detail_::IsLockableAndUnlockable<T>...>::value, "Not all types of T... are lockable and unlockable.");

		template <size_t a, size_t b, size_t step = 1>
		struct GetNext
			: std::conditional_t<a < b, std::integral_constant<size_t, a + step>, std::integral_constant<size_t, a - step>>
		{
		};

		template <size_t current, size_t target>
		struct LockImpl
		{
			template <typename tuple>
			static void Lock(tuple&& tp)
			{
				std::get<current>(tp).Lock();
				LockImpl<GetNext<current, target>::value, target>::Lock(tp);
			}

			template <typename tuple>
			static void UnLock(tuple&& tp)
			{
				std::get<current>(tp).UnLock();
				LockImpl<GetNext<current, target>::value, target>::UnLock(tp);
			}
		};

		template <size_t num>
		struct LockImpl<num, num>
		{
			template <typename tuple>
			static void Lock(tuple&& tp)
			{
				std::get<num>(tp).Lock();
			}

			template <typename tuple>
			static void UnLock(tuple&& tp)
			{
				std::get<num>(tp).UnLock();
			}
		};

	public:
		constexpr explicit natRefScopeGuard(T&... LockObjs)
			: m_RefObjs(LockObjs...)
		{
			LockImpl<0, std::tuple_size<decltype(m_RefObjs)>::value - 1>::Lock(m_RefObjs);
		}
		~natRefScopeGuard()
		{
			LockImpl<std::tuple_size<decltype(m_RefObjs)>::value - 1, 0>::UnLock(m_RefObjs);
		}

	private:
		std::tuple<T&...> m_RefObjs;
	};

	class natThreadPool final
	{
	public:
		class WorkToken final
		{
			friend class natThreadPool;
		public:
			WorkToken() = default;

			WorkToken(WorkToken&& other) noexcept
				: m_WorkThreadIndex(other.m_WorkThreadIndex), m_Result(move(other.m_Result))
			{
			}

			WorkToken& operator=(WorkToken&& other) noexcept
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

			WorkToken(nuInt workThreadId, std::future<nuInt>&& result)
				: m_WorkThreadIndex(workThreadId), m_Result(move(result))
			{
			}
		};

		typedef Delegate<nuInt(void*)> WorkFunc;
		enum : nuInt
		{
			DefaultMaxThreadCount = 4,
			Infinity = std::numeric_limits<nuInt>::max(),
		};

		explicit natThreadPool(nuInt InitialThreadCount = 0, nuInt MaxThreadCount = DefaultMaxThreadCount);
		~natThreadPool();

		void KillIdleThreads();
		void KillAllThreads();
		std::future<WorkToken> QueueWork(WorkFunc workFunc, void* param = nullptr);
		natThread::ThreadIdType GetThreadId(nuInt Index) const;
		void WaitAllJobsFinish(nuInt WaitTime = Infinity);

	private:
		class WorkerThread final
			: public natThread
		{
		public:
			WorkerThread(natThreadPool& pool, nuInt Index);
			WorkerThread(natThreadPool& pool, nuInt Index, WorkFunc CallableObj, void* Param = nullptr);
			~WorkerThread() = default;

			nBool IsIdle() const;
			std::future<nuInt> SetWork(WorkFunc CallableObj, void* Param = nullptr);

			void RequestTerminate();

		private:
			ResultType ThreadJob() override;

			natThreadPool& m_Pool;
			const nuInt m_Index;
			WorkFunc m_CallableObj;
			void* m_Arg;
			bool m_First;
			std::promise<nuInt> m_LastResult;
			std::mutex m_Mutex;
			std::condition_variable m_Cond;

			std::atomic<nBool> m_Idle, m_ShouldTerminate;
		};

		nuInt getNextAvailableIndex();
		nuInt getIdleThreadIndex();
		void onWorkerThreadIdle(nuInt Index, nBool isTerminating);

		const nuInt m_MaxThreadCount;
		std::unordered_map<nuInt, std::unique_ptr<WorkerThread>> m_Threads;
		std::queue<std::tuple<WorkFunc, void*, std::promise<WorkToken>>> m_WorkQueue;
		natCriticalSection m_Section;
	};

	///	@}
}

#ifdef _MSC_VER
#	pragma pop_macro("max")
#endif
