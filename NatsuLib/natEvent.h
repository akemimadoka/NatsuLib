////////////////////////////////////////////////////////////////////////////////
///	@file	event.h
///	@brief	Event类实现
////////////////////////////////////////////////////////////////////////////////
#pragma once

#pragma warning(push)
#pragma warning(disable : 4180)
#include <functional>
#pragma warning(pop)

#include <map>
#include <unordered_set>
#include <unordered_map>
#include "natType.h"
#include "natException.h"
#include <typeindex>

namespace Priority
{
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	Event优先级
	///	@note	为防止污染全局命名空间将其放置于Priority命名空间\n
	///			EventHandle执行优先级顺序为高-普通-低
	////////////////////////////////////////////////////////////////////////////////
	enum Priority
	{
		High = 1,	///< @brief	高优先级
		Normal = 2,	///< @brief	普通优先级
		Low = 3		///< @brief	低优先级
	};
}

namespace std
{
	template <typename Func>
	struct hash<function<Func>>
	{
		size_t operator()(function<Func> const& _Keyval) const
		{
			return hash<add_pointer_t<Func>>()(_Keyval.template target<Func>());
		}
	};

	template <typename Func>
	bool operator==(function<Func> const& left, function<Func> const& right)
	{
		return left.template target<Func>() == right.template target<Func>();
	}

	template <typename Func1, typename Func2>
	bool operator==(function<Func1> const&, function<Func2> const&)
	{
		return false;
	}
}

class natEventBase
{
public:
	natEventBase()
		: m_Canceled(false)
	{
	}
	virtual ~natEventBase() = default;

	virtual bool CanCancel() const noexcept
	{
		return false;
	}

	virtual void SetCancel(bool value) noexcept
	{
		if (CanCancel())
		{
			m_Canceled = value;
		}
	}

	virtual bool IsCanceled() const noexcept
	{
		return m_Canceled;
	}

private:
	bool m_Canceled;
};

class natEventBus final
{
public:
	typedef std::function<void(natEventBase&)> EventListenerFunc;

	NATNOINLINE static natEventBus& GetInstance()
	{
		static natEventBus s_Instance;
		return s_Instance;
	}

	template <typename EventClass>
	std::enable_if_t<std::is_base_of<natEventBase, EventClass>::value, void> RegisterEvent()
	{
		m_EventHandlerMap.try_emplace(typeid(EventClass));
	}

	template <typename EventClass, typename EventListener>
	nuInt RegisterEventListener(EventListener listener, int priority = Priority::Normal)
	{
		auto iter = m_EventHandlerMap.find(typeid(EventClass));
		if (iter == m_EventHandlerMap.end())
		{
			nat_Throw(natException, _T("Unregistered event."));
		}

		auto&& listeners = iter->second[priority];
		auto ret = listeners.empty() ? 0u : listeners.rbegin()->first + 1u;
		listeners[ret] = listener;
		return ret;
	}

	template <typename EventClass>
	void UnregisterEventListener(int priority, nuInt Handle)
	{
		auto iter = m_EventHandlerMap.find(typeid(EventClass));
		if (iter == m_EventHandlerMap.end())
		{
			nat_Throw(natException, _T("Unregistered event."));
		}

		auto listeneriter = iter->second.find(priority);
		if (listeneriter != iter->second.end())
		{
			listeneriter->second.erase(Handle);
		}
	}

	template <typename EventClass>
	bool Post(EventClass& event)
	{
		auto iter = m_EventHandlerMap.find(typeid(EventClass));
		if (iter == m_EventHandlerMap.end())
		{
			nat_Throw(natException, _T("Unregistered event."));
		}

		for (auto& listeners : iter->second)
		{
			for (auto& listener : listeners.second)
			{
				listener.second(static_cast<natEventBase&>(event));
			}
		}

		return event.IsCanceled();
	}

private:
	std::unordered_map<std::type_index, std::map<int, std::map<nuInt, std::function<void(natEventBase&)>>>> m_EventHandlerMap;

	natEventBus() = default;
	~natEventBus() = default;
};
