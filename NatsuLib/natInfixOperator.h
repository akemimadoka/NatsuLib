#pragma once

#include "natConfig.h"
#include <utility>

namespace NatsuLib
{
	namespace Detail
	{
		template <typename Op, typename T>
		struct LhsStorage
		{
			T Lhs;
		};
	}

	template <typename Op>
	struct InfixOp
	{
		constexpr InfixOp() = default;

		template <typename T>
		using LhsStorage = Detail::LhsStorage<Op, T>;
	};

	template <typename T, typename Op>
	constexpr auto operator<(T&& lhs, InfixOp<Op>) noexcept
	{
		return InfixOp<Op>::template LhsStorage<T&&>{ std::forward<T>(lhs) };
	}

	template <typename Op, typename T, typename U>
	constexpr decltype(auto) operator>(Detail::LhsStorage<Op, T>&& lhs, U&& rhs) noexcept(noexcept(Op::Apply(static_cast<T>(lhs.Lhs), std::forward<U>(rhs))))
	{
		return Op::Apply(static_cast<T>(lhs.Lhs), std::forward<U>(rhs));
	}
}
