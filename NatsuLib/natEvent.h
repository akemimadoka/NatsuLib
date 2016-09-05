////////////////////////////////////////////////////////////////////////////////
///	@file	natEvent.h
///	@brief	Event类实现
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natConfig.h"

#include <map>
#include <unordered_map>
#include <typeindex>

#include "natDelegate.h"
#include "natStringUtil.h"

namespace NatsuLib
{
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

	class natEventBase
	{
	public:
		constexpr natEventBase() noexcept
			: m_Canceled(false)
		{
		}
		virtual ~natEventBase();

		virtual nBool CanCancel() const noexcept;
		virtual void SetCancel(nBool value) noexcept;
		virtual nBool IsCanceled() const noexcept;

	private:
		nBool m_Canceled;
	};

	class natEventBus final
	{
	public:
		typedef Delegate<void(natEventBase&)> EventListenerDelegate;
		typedef nInt PriorityType;
		typedef nuInt ListenerIDType;

		template <typename EventClass>
		std::enable_if_t<std::is_base_of<natEventBase, EventClass>::value, void> RegisterEvent()
		{
			bool Succeeded;
			tie(std::ignore, Succeeded) = m_EventHandlerMap.try_emplace(typeid(EventClass));

			if (!Succeeded)
			{
				nat_Throw(natException, _T("Cannot register event \"{0}\""), natUtil::C2Wstr(typeid(EventClass).name()));
			}
		}

		template <typename EventClass>
		ListenerIDType RegisterEventListener(EventListenerDelegate const& listener, PriorityType priority = Priority::Normal)
		{
			auto iter = m_EventHandlerMap.find(typeid(EventClass));
			if (iter == m_EventHandlerMap.end())
			{
				nat_Throw(natException, _T("Unregistered event."));
			}

			auto&& listeners = iter->second[priority];
			auto ret = listeners.empty() ? 0u : listeners.rbegin()->first + 1u;
			listeners.try_emplace(ret, listener);
			return ret;
		}

		template <typename EventClass>
		void UnregisterEventListener(PriorityType priority, ListenerIDType ListenerID)
		{
			auto iter = m_EventHandlerMap.find(typeid(EventClass));
			if (iter == m_EventHandlerMap.end())
			{
				nat_Throw(natException, _T("Unregistered event."));
			}

			auto listeneriter = iter->second.find(priority);
			if (listeneriter != iter->second.end())
			{
				listeneriter->second.erase(ListenerID);
			}
		}

		template <typename EventClass>
		nBool Post(EventClass& event)
		{
			auto iter = m_EventHandlerMap.find(typeid(EventClass));
			if (iter == m_EventHandlerMap.end())
			{
				nat_Throw(natException, _T("Unregistered event."));
			}

			for (auto&& listeners : iter->second)
			{
				for (auto&& listener : listeners.second)
				{
					listener.second(static_cast<natEventBase&>(event));
				}
			}

			return event.IsCanceled();
		}

	private:
		std::unordered_map<std::type_index, std::map<PriorityType, std::map<ListenerIDType, EventListenerDelegate>>> m_EventHandlerMap;
	};
}
