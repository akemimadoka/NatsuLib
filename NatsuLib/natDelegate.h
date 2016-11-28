#pragma once
#include "natType.h"
#include <functional>
#include "natException.h"

namespace NatsuLib
{
	template <typename Func>
	class Delegate;

#ifdef _MSC_VER
#	pragma warning (push)
#	pragma warning (disable : 4521)
#endif

	template <typename Ret, typename... Args>
	class Delegate<Ret(Args...)>
	{
	public:
		Delegate() noexcept = default;
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

		Delegate& operator=(Delegate const& other) = default;
		Delegate& operator=(Delegate && other) = default;

		template <typename CallableObj>
		constexpr Delegate(CallableObj&& callableObj) noexcept
			: m_Functor(std::forward<CallableObj>(callableObj))
		{
		}

		template <typename CallableObj, typename ThisObj>
		constexpr Delegate(CallableObj&& callableObj, ThisObj&& thisObj) noexcept
			: m_Functor([callableObj, &thisObj](Args... args)
			{
				return (thisObj.*callableObj)(std::forward<Args>(args)...);
			})
		{
		}

		decltype(auto) operator()(Args... args) const
		{
			if (!m_Functor)
			{
				nat_Throw(natException, "Null delegate."_nv);
			}

			return m_Functor(std::forward<Args>(args)...);
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

#ifdef _MSC_VER
#	pragma warning (pop)
#endif
}
