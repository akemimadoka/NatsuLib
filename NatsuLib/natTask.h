#pragma once
#include "natConfig.h"
#include "natDelegate.h"
#include "natMultiThread.h"
#include <queue>
#include <future>

namespace NatsuLib
{
	class natTask
	{
	public:
		typedef nuInt TaskResultType;
		typedef void* TaskEnvironmentArgType;
		typedef Delegate<TaskResultType(TaskEnvironmentArgType)> TaskDelegate;

		natTask();
		~natTask();

		void QueueTask(TaskDelegate task, TaskEnvironmentArgType env = {});
		TaskResultType DoNext();
		std::future<TaskResultType> DoNextAsync();
		std::future<TaskResultType> DoNextAsync(natThreadPool& threadPool);
		void DoAll();
		std::future<void> DoAllAsync();
		std::future<void> DoAllAsync(natThreadPool& threadPool);
		nBool IsEmpty() const;

	private:
		std::queue<std::pair<TaskDelegate, TaskEnvironmentArgType>> m_TaskQueue;
		natCriticalSection m_CriticalSection;
	};
}
