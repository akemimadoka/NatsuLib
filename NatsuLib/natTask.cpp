#include "stdafx.h"
#include "natTask.h"
#include "natException.h"
#include "natString.h"
#include <unordered_set>

using namespace NatsuLib;

natTask::natTask()
{
}

natTask::~natTask()
{
}

void natTask::QueueTask(TaskDelegate task, TaskEnvironmentArgType env)
{
	m_TaskQueue.emplace(std::move(task), env);
}

natTask::TaskResultType natTask::DoNext()
{
	if (m_TaskQueue.empty())
	{
		nat_Throw(natException, "Task queue is empty."_nv);
	}

	const auto taskPair = move(m_TaskQueue.front());
	m_TaskQueue.pop();

	return taskPair.first(taskPair.second);
}

std::future<natTask::TaskResultType> natTask::DoNextAsync()
{
	if (m_TaskQueue.empty())
	{
		nat_Throw(natException, "Task queue is empty."_nv);
	}

	m_CriticalSection.Lock();
	const auto taskPair = move(m_TaskQueue.front());
	m_TaskQueue.pop();
	m_CriticalSection.UnLock();

	return std::async([&]
	{
		return taskPair.first(taskPair.second);
	});
}

std::future<natTask::TaskResultType> natTask::DoNextAsync(natThreadPool& threadPool)
{
	if (m_TaskQueue.empty())
	{
		nat_Throw(natException, "Task queue is empty."_nv);
	}

	m_CriticalSection.Lock();
	const auto taskPair = move(m_TaskQueue.front());
	m_TaskQueue.pop();
	m_CriticalSection.UnLock();

	return move(threadPool.QueueWork(std::move(taskPair.first), taskPair.second).get().GetResult());
}

void natTask::DoAll()
{
	while (!m_TaskQueue.empty())
	{
		DoNext();
	}
}

std::future<void> natTask::DoAllAsync()
{
	return std::async([this]
	{
		natRefScopeGuard<natCriticalSection> guard{ m_CriticalSection };
		DoAll();
	});
}

std::future<void> natTask::DoAllAsync(natThreadPool& threadPool)
{
	return std::async([&]
	{
		std::vector<std::future<natThreadPool::WorkToken>> tasks(m_TaskQueue.size());
		do
		{
			natRefScopeGuard<natCriticalSection> guard{ m_CriticalSection };
			if (m_TaskQueue.empty())
			{
				break;
			}
			const auto taskPair = move(m_TaskQueue.front());
			tasks.emplace_back(threadPool.QueueWork(std::move(taskPair.first), taskPair.second));
		} while (true);

		for (auto&& item : tasks)
		{
			item.get().GetResult().wait();
		}
	});
}

nBool natTask::IsEmpty() const
{
	return m_TaskQueue.empty();
}
