#pragma once
#include <functional>
#include <cassert>

namespace NatsuLib
{
	namespace _Detail
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
		return _Detail::apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
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

	template <typename Iter>
	class natRange
	{
	public:
		typedef typename std::iterator_traits<Iter>::iterator_category iterator_category;
		typedef typename std::iterator_traits<Iter>::value_type value_type;
		typedef typename std::iterator_traits<Iter>::difference_type difference_type;
		typedef typename std::iterator_traits<Iter>::reference reference;
		typedef typename std::iterator_traits<Iter>::pointer pointer;

		constexpr natRange(Iter begin, Iter end)
			: m_IterBegin(begin), m_IterEnd(end)
		{
		}

		template <typename R>
		constexpr explicit natRange(R&& range)
			: m_IterBegin(std::begin(range)), m_IterEnd(std::end(range))
		{
		}

		~natRange() = default;

		constexpr Iter begin() const
		{
			return m_IterBegin;
		}

		constexpr Iter end() const
		{
			return m_IterEnd;
		}

		constexpr reference front() const
		{
			assert(!empty());

			return *m_IterBegin;
		}

		constexpr reference back() const
		{
			assert(!empty());

			return *std::prev(m_IterEnd);
		}

		constexpr reference operator[](difference_type index) const
		{
			assert(index < size());

			return *std::next(m_IterBegin, index);
		}

		constexpr bool empty() const
		{
			return m_IterBegin == m_IterEnd;
		}

		constexpr difference_type size() const
		{
			return std::distance(m_IterBegin, m_IterEnd);
		}

		natRange& pop_front()
		{
			return pop_front(1);
		}

		natRange& pop_front(difference_type n)
		{
			assert(size() > n - 1);
			std::advance(m_IterBegin, n);
			return *this;
		}

		natRange& pop_front_upto(difference_type n)
		{
			return pop_front(size() - n);
		}

		natRange& pop_back()
		{
			return pop_back(1);
		}

		natRange& pop_back(difference_type n)
		{
			assert(size() > n - 1);
			std::advance(m_IterEnd, -n);
			return *this;
		}

		natRange& pop_back_upto(difference_type n)
		{
			return pop_back(n - size());
		}

		std::pair<natRange, natRange> split(difference_type index) const
		{
			assert(index < size());

			auto&& MidIter = std::next(m_IterBegin, index);
			return std::make_pair(natRange(m_IterBegin, MidIter), natRange(MidIter, m_IterEnd));
		}

		natRange slice(difference_type start, difference_type stop) const
		{
			assert(stop >= start);
			assert(stop - start <= size());

			return natRange(std::next(m_IterBegin, start), std::next(m_IterBegin, stop));
		}

		natRange slice(difference_type start) const
		{
			return slice(start, size() - start);
		}

	private:
		Iter m_IterBegin, m_IterEnd;
	};

	template<typename Iter>
	constexpr natRange<Iter> make_range(Iter begin, Iter end)
	{
		return natRange<Iter>(begin, end);
	}

	template<typename Range>
	constexpr auto make_range(Range && r)
	{
		return natRange<decltype(std::begin(r))>(r);
	}

	template<typename Range>
	constexpr auto make_ptr_range(Range && r)
	{
		return natRange<decltype(&*std::begin(r))>(r);
	}
}
