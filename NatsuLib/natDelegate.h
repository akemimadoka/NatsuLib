#pragma once
#include "natType.h"
#include <functional>

namespace NatsuLib
{
	template <typename Func>
	class Delegate;

#pragma warning (push)
#pragma warning (disable : 4521)

	template <typename Ret, typename... Args>
	class Delegate<Ret(Args...)>
	{
	public:
		Delegate(Delegate const& other) noexcept
			: m_Functor(other.m_Functor)
		{
		}
		Delegate(Delegate& other) noexcept
			: m_Functor(other.m_Functor)
		{
		}
		Delegate(Delegate&& other) noexcept
			: m_Functor(std::move(other.m_Functor))
		{
		}

		template <typename CallableObj>
		constexpr Delegate(CallableObj&& callableObj) noexcept
			: m_Functor(std::forward<CallableObj>(callableObj))
		{
		}

		template <typename CallableObj, typename ThisObj>
		constexpr Delegate(CallableObj&& callableObj, ThisObj&& thisObj) noexcept
			: m_Functor([callableObj, &thisObj](Args&&... args)
			{
				(thisObj.*callableObj)(std::forward<Args>(args)...);
			})
		{
		}

		Ret operator()(Args&&... args)
		{
			return m_Functor(std::forward<Args>(args)...);
		}

	private:
		std::function<Ret(Args...)> m_Functor;
	};

#pragma warning (pop)

}
