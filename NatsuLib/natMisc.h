#pragma once
#include <functional>

namespace detail
{
	template <class F, class Tuple, std::size_t... I>
	constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
	{
		return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
	}
}

template <class F, class Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t)
{
	return detail::apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
		std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>{}>{});
}

template <typename T, typename... Args>
class natScope
{
public:
	constexpr explicit natScope(T&& CallableObj, Args&&... Arg)
		: m_CallableObj(std::forward<T>(CallableObj)), m_Args(std::forward_as_tuple<Args>(Arg)...)
	{
	}

	~natScope()
	{
		apply(m_CallableObj, m_Args);
	}

private:
	T m_CallableObj;
	std::tuple<Args...> m_Args;
};
