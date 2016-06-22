////////////////////////////////////////////////////////////////////////////////
///	@file	natMultiThread.h
///	@brief	多线程支持
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <Windows.h>

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
	///	@brief	构造函数
	///	@param[in]	Pause	创建线程时暂停
	explicit natThread(nBool Pause = true);
	virtual ~natThread();

	///	@brief		获得线程句柄
	///	@warning	请勿手动删除
	HANDLE GetHandle() const;

	///	@brief	继续线程执行
	///	@return	是否成功
	nBool Resume();

	///	@brief	阻塞线程执行
	///	@return	是否成功
	nBool Suspend();

	///	@brief	等待线程执行
	///	@param[in]	WaitTime	等待时间
	///	@return	等待线程状态
	DWORD Wait(nuInt WaitTime = INFINITE);

	///	@brief	结束线程
	///	@param[in]	ExitCode	退出码
	///	@return	是否成功
	nBool Terminate(nuInt ExitCode = nuInt(-1));

	///	@brief	获得退出码
	nuInt GetExitCode() const;
protected:
	///	@brief	重写此方法以实现线程工作
	virtual nuInt ThreadJob() = 0;

private:
	///	@brief	执行线程的包装函数
	///	@param[in]	p	指向Thread类的指针
	static DWORD WINAPI execute(void* p);
	HANDLE m_hThread;
	DWORD m_hThreadID;
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
///	@note	通过Event对多线程操作进行同步\n
///			需与本程序的Event区分
////////////////////////////////////////////////////////////////////////////////
class natEventWrapper final
{
public:
	natEventWrapper(nBool AutoReset, nBool InitialState);
	~natEventWrapper();

	///	@brief		获得句柄
	///	@warning	请勿手动释放
	HANDLE GetHandle() const;

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
	nBool Wait(nuInt WaitTime = INFINITE) const;
private:
	HANDLE m_hEvent;
};

///	@}