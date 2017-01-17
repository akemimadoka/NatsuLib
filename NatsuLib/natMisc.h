#pragma once
#include "natType.h"
#include <functional>
#include <cassert>

namespace NatsuLib
{
	namespace detail_
	{
		template <class F, class Tuple, size_t... I>
		constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
		{
			return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
		}

		struct nullopt_t {};
		constexpr nullopt_t nullopt{};

		struct defaultconstruct_t {};
		constexpr defaultconstruct_t defaultconstruct{};

		[[noreturn]] void NotConstructed();
		[[noreturn]] void ValueNotAvailable();

#ifdef _MSC_VER
#pragma pack(1)
#endif
		template <typename T>
		class CommonStorage final
		{
		public:
			constexpr CommonStorage() noexcept
				: m_Constructed(false), m_Storage{0}
			{
			}

			CommonStorage(CommonStorage const& other) noexcept(noexcept(std::declval<CommonStorage<T>>().Init(std::declval<T const&>())))
				: CommonStorage()
			{
				if (other.m_Constructed)
				{
					Init(other.Get());
				}
			}

			CommonStorage(CommonStorage && other) noexcept(noexcept(std::declval<CommonStorage<T>>().Init(std::declval<T &&>())))
				: CommonStorage()
			{
				if (other.m_Constructed)
				{
					Init(std::move(other.Get()));
				}
			}

			constexpr explicit CommonStorage(defaultconstruct_t) noexcept(std::is_nothrow_default_constructible<T>::value)
				: CommonStorage()
			{
				Init(defaultconstruct);
			}

			constexpr explicit CommonStorage(const T& obj) noexcept(noexcept(std::declval<CommonStorage<T>>().Init(obj)))
				: CommonStorage()
			{
				Init(obj);
			}

			constexpr explicit CommonStorage(T&& obj) noexcept(noexcept(std::declval<CommonStorage<T>>().Init(std::move(obj))))
				: CommonStorage()
			{
				Init(std::move(obj));
			}

			template <typename... Args>
			constexpr explicit CommonStorage(Args&&... args) noexcept(noexcept(std::declval<CommonStorage<T>>().Init(std::forward<Args>(args)...)))
				: CommonStorage()
			{
				Init(std::forward<Args>(args)...);
			}

			~CommonStorage()
			{
				Init();
			}

			void Init() noexcept
			{
				Uninit(std::is_destructible<T>{});
			}

			void Uninit(std::true_type) noexcept
			{
				if (m_Constructed)
				{
					Get().~T();
					m_Constructed = false;
				}
			}

			void Uninit(std::false_type) noexcept
			{
				m_Constructed = false;
			}

			void Init(defaultconstruct_t) noexcept(std::is_nothrow_default_constructible<T>::value)
			{
				Init();

				new(m_Storage) T;
				m_Constructed = true;
			}

			void Init(const T& obj) noexcept(std::is_nothrow_copy_constructible<T>::value)
			{
				Init();

				new(m_Storage) T(obj);
				m_Constructed = true;
			}

			void Init(T&& obj) noexcept(std::is_nothrow_move_constructible<T>::value)
			{
				Init();

				new(m_Storage) T(std::move(obj));
				m_Constructed = true;
			}

			template <typename... Args>
			void Init(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args&&...>::value)
			{
				Init();

				new(m_Storage) T(std::forward<Args>(args)...);
				m_Constructed = true;
			}

			T const& Get() const&
			{
				if (!m_Constructed)
				{
					NotConstructed();
				}
				return Get(std::nothrow);
			}

			T const& Get(std::nothrow_t) const&
			{
				return *reinterpret_cast<const T*>(m_Storage);
			}

			T& Get() &
			{
				if (!m_Constructed)
				{
					NotConstructed();
				}
				return Get(std::nothrow);
			}

			T& Get(std::nothrow_t) &
			{
				return *reinterpret_cast<T*>(m_Storage);
			}

			T const&& Get() const&&
			{
				if (!m_Constructed)
				{
					NotConstructed();
				}
				return std::move(Get(std::nothrow));
			}

			T const&& Get(std::nothrow_t) const&&
			{
				return std::move(*reinterpret_cast<const T*>(m_Storage));
			}

			T&& Get() &&
			{
				if (!m_Constructed)
				{
					NotConstructed();
				}
				return std::move(Get(std::nothrow));
			}

			T&& Get(std::nothrow_t) &&
			{
				return std::move(*reinterpret_cast<T*>(m_Storage));
			}

			constexpr nBool Constructed() const noexcept
			{
				return m_Constructed;
			}

		private:
			nBool m_Constructed;
			nByte m_Storage[sizeof(T)];
		};
#ifdef _MSC_VER
#pragma pack()
#endif
	}

	template <typename T, typename Self>
	using NonSelf = std::bool_constant<!std::is_same<std::decay_t<T>, Self>::value && !std::is_base_of<Self, std::decay_t<T>>::value>;

	template <class F, class Tuple>
	constexpr decltype(auto) apply(F&& f, Tuple&& t)
	{
		return detail_::apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
			std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
	}

	template <typename T, typename... Args>
	class natScope final
	{
	public:
		constexpr explicit natScope(T CallableObj, Args&&... args)
			: m_ShouldCall(true), m_CallableObj(CallableObj), m_Args(std::forward<Args>(args)...)
		{
		}

		constexpr natScope(natScope && other) noexcept
			: m_ShouldCall(true), m_CallableObj(std::move(other.m_CallableObj)), m_Args(std::move(other.m_Args))
		{
			other.m_ShouldCall = false;
		}

		~natScope()
		{
			if (m_ShouldCall)
			{
				apply(m_CallableObj, m_Args);
			}
		}

	private:
		nBool m_ShouldCall;
		T m_CallableObj;
		std::tuple<Args&&...> m_Args;
	};
	
	template <typename T, typename ...Args>
	constexpr auto make_scope(T CallableObj, Args&&... args)
	{
		return natScope<T, Args&&...>{ CallableObj, std::forward<Args>(args)... };
	}

	using detail_::nullopt_t;
	using detail_::nullopt;

	using detail_::defaultconstruct_t;
	using detail_::defaultconstruct;

	template <typename T>
	class Optional final
	{
	public:
		typedef T value_type;

		constexpr Optional()
		{
		}
		constexpr Optional(nullopt_t)
		{
		}
		constexpr Optional(defaultconstruct_t)
			: m_Value(defaultconstruct)
		{
		}
		Optional(Optional const& other)
			: m_Value(other.m_Value)
		{
		}
		Optional(Optional && other) noexcept
			: m_Value(std::move(other.m_Value))
		{
		}
		constexpr Optional(T const& value)
			: m_Value(value)
		{
		}
		constexpr Optional(T && value)
			: m_Value(std::move(value))
		{
		}
		template <typename... Args>
		constexpr Optional(Args&&... args)
			: m_Value(std::forward<Args>(args)...)
		{
		}

		Optional& operator=(nullopt_t) &
		{
			m_Value.Init();
			return *this;
		}

		Optional& operator=(Optional const& other) &
		{
			if (m_Value.Constructed())
			{
				if (other.m_Value.Constructed())
				{
					m_Value.Get() = other.m_Value.Get();
				}
				else
				{
					m_Value.Init();
				}
			}
			else
			{
				if (other.m_Value.Constructed())
				{
					m_Value.Init(other.m_Value.Get());
				}
			}
			
			return *this;
		}

		Optional& operator=(Optional && other) & noexcept
		{
			if (m_Value.Constructed())
			{
				if (other.m_Value.Constructed())
				{
					m_Value.Get() = other.m_Value.Get();
				}
				else
				{
					m_Value.Init();
				}
			}
			else
			{
				if (other.m_Value.Constructed())
				{
					m_Value.Init(other.m_Value.Get());
				}
			}
			
			return *this;
		}

		template <typename U>
		std::enable_if_t<std::is_same<std::decay_t<U>, T>::value, Optional&> operator=(U&& value) &
		{
			if (m_Value.Constructed())
			{
				m_Value.Get() = std::forward<U>(value);
			}
			else
			{
				m_Value.Init(std::forward<U>(value));
			}

			return *this;
		}

		const T* operator->() const
		{
			assert(has_value() && "There is no available value.");

			return &m_Value.Get();
		}

		T* operator->()
		{
			assert(has_value() && "There is no available value.");

			return &m_Value.Get();
		}

		T const& operator*() const&
		{
			assert(has_value() && "There is no available value.");

			return m_Value.Get();
		}

		T& operator*() &
		{
			assert(has_value() && "There is no available value.");

			return m_Value.Get();
		}

		T const&& operator*() const&&
		{
			assert(has_value() && "There is no available value.");

			return std::move(m_Value.Get());
		}

		T&& operator*() &&
		{
			assert(has_value() && "There is no available value.");

			return std::move(m_Value.Get());
		}

		constexpr explicit operator nBool() const
		{
			return has_value();
		}

		constexpr nBool has_value() const
		{
			return m_Value.Constructed();
		}

		T const& value() const&
		{
			if (!has_value())
			{
				detail_::ValueNotAvailable();
			}

			return m_Value.Get();
		}

		T& value() &
		{
			if (!has_value())
			{
				detail_::ValueNotAvailable();
			}

			return m_Value.Get();
		}

		T const&& value() const&&
		{
			if (!has_value())
			{
				detail_::ValueNotAvailable();
			}

			return std::move(m_Value.Get());
		}

		T&& value() &&
		{
			if (!has_value())
			{
				detail_::ValueNotAvailable();
			}

			return std::move(m_Value.Get());
		}

		template <typename U>
		constexpr T value_or(U&& default_value) const&
		{
			return has_value() ? m_Value.Get() : static_cast<T>(std::forward<U>(default_value));
		}

		template <typename U>
		T value_or(U&& default_value) &&
		{
			return has_value() ? std::move(m_Value.Get()) : static_cast<T>(std::forward<U>(default_value));
		}

		void swap(Optional& other) noexcept(std::is_nothrow_move_constructible<T>::value && noexcept(std::swap(std::declval<T&>(), std::declval<T&>())))
		{
			if (has_value() && !other.has_value())
			{
				other.m_Value.Init(std::move(m_Value.Get()));
				m_Value.Init();
			}
			else if (!has_value() && other.has_value())
			{
				m_Value.Init(std::move(other.m_Value.Get()));
				other.m_Value.Init();
			}
			else if (has_value() && other.has_value())
			{
				std::swap(m_Value.Get(), other.m_Value.Get());
			}
		}

		void reset()
		{
			m_Value.Init();
		}

		template <typename... Args>
		void emplace(Args&&... args)
		{
			m_Value.Init(std::forward<Args>(args)...);
		}

	private:
		detail_::CommonStorage<T> m_Value;
	};

	template <typename Iter>
	class Range final
	{
	public:
		typedef typename std::iterator_traits<Iter>::iterator_category iterator_category;
		typedef typename std::iterator_traits<Iter>::value_type value_type;
		typedef typename std::iterator_traits<Iter>::difference_type difference_type;
		typedef typename std::iterator_traits<Iter>::reference reference;
		typedef typename std::iterator_traits<Iter>::pointer pointer;

		constexpr Range(Iter begin, Iter end)
			: m_IterBegin(begin), m_IterEnd(end)
		{
			//assert(size() >= 0);
		}

		template <typename R>
		constexpr explicit Range(R&& range)
			: m_IterBegin(std::begin(range)), m_IterEnd(std::end(range))
		{
			//assert(size() >= 0);
		}

		~Range() = default;

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
			//assert(index < size());

			return *std::next(m_IterBegin, index);
		}

		constexpr nBool empty() const
		{
			return m_IterBegin == m_IterEnd;
		}

		constexpr difference_type size() const
		{
			return std::distance(m_IterBegin, m_IterEnd);
		}

		Range& pop_front()
		{
			return pop_front(1);
		}

		Range& pop_front(difference_type n)
		{
			//assert(size() >= n);
			std::advance(m_IterBegin, n);
			return *this;
		}

		Range& pop_front_upto(difference_type n)
		{
			return pop_front(size() - n);
		}

		Range& pop_back()
		{
			return pop_back(1);
		}

		Range& pop_back(difference_type n)
		{
			//assert(size() >= n);
			std::advance(m_IterEnd, -n);
			return *this;
		}

		Range& pop_back_upto(difference_type n)
		{
			return pop_back(n - size());
		}

		std::pair<Range, Range> split(difference_type index) const
		{
			//assert(index < size());

			auto&& MidIter = std::next(m_IterBegin, index);
			return std::make_pair(Range(m_IterBegin, MidIter), Range(MidIter, m_IterEnd));
		}

		Range slice(difference_type start, difference_type stop) const
		{
			assert(stop >= start);
			//assert(stop - start <= size());

			return Range(std::next(m_IterBegin, start), std::next(m_IterBegin, stop));
		}

		Range slice(difference_type start) const
		{
			return slice(start, size() - start);
		}

	private:
		void Init(Iter begin, Iter end)
		{
			//assert(std::distance(begin, end) >= 0);

			m_IterBegin = std::move(begin);
			m_IterEnd = std::move(end);
		}

		Iter m_IterBegin, m_IterEnd;
	};

	template <typename Iter>
	class natRange_ptrIterator final
	{
	public:
		typedef typename std::iterator_traits<Iter>::iterator_category iterator_category;
		typedef typename std::iterator_traits<Iter>::value_type value_type;
		typedef typename std::iterator_traits<Iter>::difference_type difference_type;
		typedef typename std::iterator_traits<Iter>::reference reference;
		typedef typename std::iterator_traits<Iter>::pointer pointer;

		constexpr explicit natRange_ptrIterator(Iter iter)
			: m_Iter(iter)
		{
		}

		constexpr operator pointer() const
		{
			return std::addressof(*m_Iter);
		}

		decltype(auto) operator*() const
		{
			return *m_Iter;
		}

		natRange_ptrIterator& operator++() &
		{
			return *this += 1;
		}

		natRange_ptrIterator& operator--() &
		{
			return *this -= 1;
		}

		natRange_ptrIterator operator+(difference_type n) const
		{
			return natRange_ptrIterator(std::next(m_Iter, n));
		}

		natRange_ptrIterator operator-(difference_type n) const
		{
			return natRange_ptrIterator(std::prev(m_Iter, n));
		}

		natRange_ptrIterator& operator+=(difference_type n) &
		{
			std::advance(m_Iter, n);
			return *this;
		}

		natRange_ptrIterator& operator-=(difference_type n) &
		{
			std::advance(m_Iter, -n);
			return *this;
		}

		decltype(auto) operator[](difference_type n) const
		{
			return *std::next(m_Iter, n);
		}

		nBool operator<=(natRange_ptrIterator const& other) const
		{
			return std::distance(m_Iter, other.m_Iter) <= 0;
		}

		nBool operator>=(natRange_ptrIterator const& other) const
		{
			return std::distance(m_Iter, other.m_Iter) >= 0;
		}

		nBool operator==(natRange_ptrIterator const& other) const
		{
			return m_Iter == other.m_Iter;
		}

		nBool operator<(natRange_ptrIterator const& other) const
		{
			return !(*this >= other);
		}

		nBool operator>(natRange_ptrIterator const& other) const
		{
			return !(*this <= other);
		}
	private:
		Iter m_Iter;
	};

	template<typename Iter>
	constexpr auto make_range(Iter begin, Iter end)
	{
		return Range<Iter>(begin, end);
	}

	template<typename Range_t>
	constexpr auto make_range(Range_t && r)
	{
		return Range<decltype(std::begin(r))>(r);
	}

	template<typename Range_t>
	constexpr auto make_ptr_range(Range_t && r)
	{
		return Range<natRange_ptrIterator<decltype(std::begin(r))>>(r);
	}

	class noncopyable
	{
	public:
		noncopyable() = default;

		noncopyable(noncopyable const&) = delete;
		noncopyable& operator=(noncopyable const&) = delete;

	protected:
		~noncopyable() = default;
	};

	class nonmovable
	{
	public:
		nonmovable() = default;

		nonmovable(nonmovable &&) = delete;
		nonmovable& operator=(nonmovable &&) = delete;

	protected:
		~nonmovable() = default;
	};
}

namespace std
{
	template <typename T>
	struct hash<NatsuLib::Optional<T>>
	{
		size_t operator()(NatsuLib::Optional<T> const& _Keyval) const
		{
			return _Keyval ? hash<T>{}(*_Keyval) : 0u;
		}
	};
}
