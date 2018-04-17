#pragma once
#include "natType.h"
#include <functional>
#include "natException.h"

namespace NatsuLib
{
	template <typename Func>
	class Delegate;

	template <typename Ret, typename... Args>
	class Delegate<Ret(Args...)>
	{
	public:
		Delegate() noexcept = default;

		template <typename CallableObj, std::enable_if_t<NonSelf<CallableObj, Delegate>::value, int> = 0>
		constexpr Delegate(CallableObj&& callableObj) noexcept
			: m_Functor(std::forward<CallableObj>(callableObj))
		{
		}

		template <typename CallableObj, typename ThisObj>
		constexpr Delegate(CallableObj&& callableObj, ThisObj&& thisObj) noexcept
			: m_Functor([callableObj, &thisObj](Args... args)
			{
				return (thisObj.*callableObj)(static_cast<Args>(args)...);
			})
		{
		}

		decltype(auto) operator()(Args... args) const
		{
			if (!m_Functor)
			{
				nat_Throw(natException, "Null delegate."_nv);
			}

			return m_Functor(static_cast<Args>(args)...);
		}

		explicit operator bool() const noexcept
		{
			return static_cast<bool>(m_Functor);
		}

		nBool operator!() const noexcept
		{
			return !m_Functor;
		}

	private:
		std::function<Ret(Args...)> m_Functor;
	};
}
