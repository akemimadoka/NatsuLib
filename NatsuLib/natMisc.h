////////////////////////////////////////////////////////////////////////////////
///	@file	natMisc.h
///	@brief	NatsuLib杂项工具
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natType.h"
#include <functional>
#include <cassert>
#include <tuple>

#define MAKE_ENUM_CLASS_BITMASK_TYPE(enumName) static_assert(std::is_enum<enumName>::value, "enumName is not a enum.");\
	constexpr enumName operator|(enumName a, enumName b) noexcept\
	{\
		typedef std::underlying_type_t<enumName> underlying_type;\
		return static_cast<enumName>(static_cast<underlying_type>(a) | static_cast<underlying_type>(b));\
	}\
	constexpr enumName operator&(enumName a, enumName b) noexcept\
	{\
		typedef std::underlying_type_t<enumName> underlying_type;\
		return static_cast<enumName>(static_cast<underlying_type>(a) & static_cast<underlying_type>(b));\
	}\
	constexpr enumName operator^(enumName a, enumName b) noexcept\
	{\
		typedef std::underlying_type_t<enumName> underlying_type;\
		return static_cast<enumName>(static_cast<underlying_type>(a) ^ static_cast<underlying_type>(b));\
	}\
	constexpr enumName operator~(enumName a) noexcept\
	{\
		typedef std::underlying_type_t<enumName> underlying_type;\
		return static_cast<enumName>(~static_cast<underlying_type>(a));\
	}\
	constexpr enumName& operator|=(enumName& a, enumName b) noexcept\
	{\
		return a = (a | b);\
	}\
	constexpr enumName& operator&=(enumName& a, enumName b) noexcept\
	{\
		return a = (a & b);\
	}\
	constexpr enumName& operator^=(enumName& a, enumName b) noexcept\
	{\
		return a = (a ^ b);\
	}

namespace NatsuLib
{
	template <typename Enum>
	constexpr std::enable_if_t<std::is_enum_v<Enum>, nBool> HasAnyFlags(Enum test, Enum flags) noexcept
	{
		using UnderlyingType = std::underlying_type_t<Enum>;
		return (static_cast<UnderlyingType>(test) & static_cast<UnderlyingType>(flags)) != UnderlyingType{};
	}

	template <typename Enum>
	constexpr std::enable_if_t<std::is_enum_v<Enum>, nBool> HasAllFlags(Enum test, Enum flags) noexcept
	{
		using UnderlyingType = std::underlying_type_t<Enum>;
		return (static_cast<UnderlyingType>(test) & static_cast<UnderlyingType>(flags)) == static_cast<UnderlyingType>(flags);
	}

	namespace detail_
	{
		struct nullopt_t { constexpr nullopt_t() = default; };
		constexpr nullopt_t nullopt{};

		struct defaultconstruct_t { constexpr defaultconstruct_t() = default; };
		constexpr defaultconstruct_t defaultconstruct{};

		struct in_place_t { constexpr in_place_t() = default; };
		constexpr in_place_t in_place;

		[[noreturn]] void NotConstructed();
		[[noreturn]] void ValueNotAvailable();

		template <typename T>
		class CommonStorage final
		{
		public:
			constexpr CommonStorage() noexcept
				: m_Constructed(false), m_Storage{}
			{
			}

			CommonStorage(CommonStorage const& other) noexcept(noexcept(std::declval<CommonStorage<T>>().Init(std::declval<T const&>())))
				: m_Constructed(false), m_Storage{}
			{
				if (other.m_Constructed)
				{
					Init(other.Get());
				}
			}

			CommonStorage(CommonStorage && other) noexcept(noexcept(std::declval<CommonStorage<T>>().Init(std::declval<T &&>())))
				: m_Constructed(false), m_Storage{}
			{
				if (other.m_Constructed)
				{
					Init(std::move(other.Get()));
				}
			}

			constexpr explicit CommonStorage(defaultconstruct_t) noexcept(std::is_nothrow_default_constructible<T>::value)
				: m_Constructed(false), m_Storage{}
			{
				Init(defaultconstruct);
			}

			constexpr explicit CommonStorage(const T& obj) noexcept(noexcept(std::declval<CommonStorage<T>>().Init(obj)))
				: m_Constructed(false), m_Storage{}
			{
				Init(obj);
			}

			constexpr explicit CommonStorage(T&& obj) noexcept(noexcept(std::declval<CommonStorage<T>>().Init(std::move(obj))))
				: m_Constructed(false), m_Storage{}
			{
				Init(std::move(obj));
			}

			template <typename... Args>
			constexpr explicit CommonStorage(in_place_t, Args&&... args) noexcept(noexcept(std::declval<CommonStorage<T>>().Init(std::forward<Args>(args)...)))
				: m_Constructed(false), m_Storage{}
			{
				Init(std::forward<Args>(args)...);
			}

			~CommonStorage()
			{
				Init();
			}

			constexpr void Init() noexcept
			{
				Uninit(std::is_destructible<T>{});
			}

			constexpr void Uninit(std::true_type) noexcept
			{
				if (m_Constructed)
				{
					Get().~T();
					m_Constructed = false;
				}
			}

			constexpr void Uninit(std::false_type) noexcept
			{
				m_Constructed = false;
			}

			constexpr void Init(defaultconstruct_t) noexcept(std::is_nothrow_default_constructible<T>::value)
			{
				Init();

				new(m_Storage) T;
				m_Constructed = true;
			}

			constexpr void Init(const T& obj) noexcept(std::is_nothrow_copy_constructible<T>::value)
			{
				Init();

				new(m_Storage) T(obj);
				m_Constructed = true;
			}

			constexpr void Init(T&& obj) noexcept(std::is_nothrow_move_constructible<T>::value)
			{
				Init();

				new(m_Storage) T(std::move(obj));
				m_Constructed = true;
			}

			template <typename... Args>
			constexpr void Init(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args&&...>::value)
			{
				Init();

				new(m_Storage) T(std::forward<Args>(args)...);
				m_Constructed = true;
			}

			constexpr T const& Get() const&
			{
				if (!m_Constructed)
				{
					NotConstructed();
				}
				return Get(std::nothrow);
			}

			constexpr T const& Get(std::nothrow_t) const&
			{
				return *reinterpret_cast<const T*>(m_Storage);
			}

			constexpr T& Get() &
			{
				if (!m_Constructed)
				{
					NotConstructed();
				}
				return Get(std::nothrow);
			}

			constexpr T& Get(std::nothrow_t) &
			{
				return *reinterpret_cast<T*>(m_Storage);
			}

			constexpr T const&& Get() const&&
			{
				if (!m_Constructed)
				{
					NotConstructed();
				}
				return std::move(Get(std::nothrow));
			}

			constexpr T const&& Get(std::nothrow_t) const&&
			{
				return std::move(*reinterpret_cast<const T*>(m_Storage));
			}

			constexpr T&& Get() &&
			{
				if (!m_Constructed)
				{
					NotConstructed();
				}
				return std::move(Get(std::nothrow));
			}

			constexpr T&& Get(std::nothrow_t) &&
			{
				return std::move(*reinterpret_cast<T*>(m_Storage));
			}

			constexpr nBool Constructed() const noexcept
			{
				return m_Constructed;
			}

		private:
			nBool m_Constructed;
			alignas(T) nByte m_Storage[sizeof(T)];
		};

		struct OnlyFail_t
		{
			constexpr OnlyFail_t() = default;
		};

		constexpr OnlyFail_t OnlyFail{};
	}

	using detail_::OnlyFail_t;
	using detail_::OnlyFail;

	template <typename T, typename Self>
	using NonSelf = std::bool_constant<!std::is_same<std::decay_t<T>, Self>::value && !std::is_base_of<Self, std::decay_t<T>>::value>;

	///	@brief	自动回调域
	///	@remark	用于在析构时执行特定操作
	template <typename T, typename... Args>
	class natScope final
	{
	public:
		constexpr explicit natScope(T CallableObj, Args&&... args)
			: m_OnlyFail{ false }, m_ShouldCall(true), m_CallableObj(std::move(CallableObj)), m_Args(std::forward<Args>(args)...)
		{
		}

		constexpr explicit natScope(OnlyFail_t, T CallableObj, Args&&... args)
			: m_OnlyFail{ true }, m_ShouldCall(true), m_CallableObj(std::move(CallableObj)), m_Args(std::forward<Args>(args)...)
		{
		}

		constexpr natScope(natScope && other) noexcept
			: m_OnlyFail{ other.m_OnlyFail }, m_ShouldCall(true), m_CallableObj(std::move(other.m_CallableObj)), m_Args(std::move(other.m_Args))
		{
			other.m_ShouldCall = false;
		}

		~natScope()
		{
			if (m_ShouldCall && (!m_OnlyFail || std::uncaught_exceptions()))
			{
				std::apply(m_CallableObj, m_Args);
			}
		}

		void SetShouldCall(nBool value) noexcept
		{
			m_ShouldCall = value;
		}

	private:
		const nBool m_OnlyFail;
		nBool m_ShouldCall;
		T m_CallableObj;
		std::tuple<Args &&...> m_Args;
	};

	template <typename T, typename ...Args>
	constexpr auto make_scope(T CallableObj, Args&&... args)
	{
		return natScope<T, decltype(args)...>{ std::move(CallableObj), std::forward<Args>(args)... };
	}

	using detail_::nullopt_t;
	using detail_::nullopt;

	using detail_::defaultconstruct_t;
	using detail_::defaultconstruct;

	using detail_::in_place_t;
	using detail_::in_place;

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	可空类型
	///	@note	由于std::optional进入C++17，此类已无必要继续使用，为了兼容性而保留
	////////////////////////////////////////////////////////////////////////////////
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
		constexpr Optional(in_place_t, Args&&... args)
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
					m_Value.Get() = std::move(other.m_Value.Get());
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
					m_Value.Init(std::move(other.m_Value.Get()));
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

		constexpr const T* operator->() const
		{
			assert(has_value() && "There is no available value.");

			return &m_Value.Get();
		}

		constexpr T* operator->()
		{
			assert(has_value() && "There is no available value.");

			return &m_Value.Get();
		}

		constexpr T const& operator*() const&
		{
			assert(has_value() && "There is no available value.");

			return m_Value.Get();
		}

		constexpr T& operator*() &
		{
			assert(has_value() && "There is no available value.");

			return m_Value.Get();
		}

		constexpr T const&& operator*() const&&
		{
			assert(has_value() && "There is no available value.");

			return std::move(m_Value.Get());
		}

		constexpr T&& operator*() &&
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

		constexpr T const& value() const&
		{
			if (!has_value())
			{
				detail_::ValueNotAvailable();
			}

			return m_Value.Get();
		}

		constexpr T& value() &
		{
			if (!has_value())
			{
				detail_::ValueNotAvailable();
			}

			return m_Value.Get();
		}

		constexpr T const&& value() const&&
		{
			if (!has_value())
			{
				detail_::ValueNotAvailable();
			}

			return std::move(m_Value.Get());
		}

		constexpr T&& value() &&
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
		constexpr T value_or(U&& default_value) &&
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

		void emplace()
		{
			m_Value.Init(defaultconstruct);
		}

	private:
		detail_::CommonStorage<T> m_Value;
	};

	///	@brief	柯里化
	template <typename Callable, typename... Args>
	class Currier
	{
	public:
		constexpr explicit Currier(Callable&& callableObj, std::tuple<Args&&...>&& argsTuple) noexcept
			: m_CallableObj{ std::forward<Callable>(callableObj) }, m_ArgsTuple{ std::move(argsTuple) }
		{
		}

		template <typename Arg>
		constexpr decltype(auto) operator()(Arg&& arg) const noexcept(!std::is_invocable_v<Callable&&, Args&&..., Arg&&> || std::is_nothrow_invocable_v<Callable&&, Args&&..., Arg&&>)
		{
			if constexpr (std::is_invocable_v<Callable&&, Args&&..., Arg&&>)
			{
				return std::apply(m_CallableObj, std::tuple_cat(std::move(m_ArgsTuple), std::tuple<Arg&&>(std::forward<Arg>(arg))));
			}
			else
			{
				return Currier<Callable&&, Args&&..., Arg&&>{ static_cast<Callable&&>(m_CallableObj), std::tuple_cat(std::move(m_ArgsTuple), std::tuple<Arg&&>(std::forward<Arg>(arg))) };
			}
		}

	private:
		Callable && m_CallableObj;
		std::tuple<Args&&...> m_ArgsTuple;
	};

	template <typename Callable>
	constexpr auto MakeCurrier(Callable&& callableObj) noexcept
	{
		return Currier<Callable&&>{ std::forward<Callable>(callableObj), std::tuple<>{} };
	}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	范围
	////////////////////////////////////////////////////////////////////////////////
	template <typename Iter>
	class Range final
	{
	public:
		typedef Iter iterator;
		typedef typename std::iterator_traits<Iter>::value_type value_type;
		typedef typename std::iterator_traits<Iter>::difference_type difference_type;
		typedef typename std::iterator_traits<Iter>::reference reference;
		typedef std::make_unsigned_t<difference_type> size_type;

		constexpr Range(Iter begin, Iter end)
			: m_IterBegin(std::move(begin)), m_IterEnd(std::move(end))
		{
			//assert(size() >= 0);
		}

		constexpr Range(Range const&) = default;
		constexpr Range(Range&&) = default;

		Range& operator=(Range const&) = default;
		Range& operator=(Range &&) = default;

		template <typename R>
		constexpr explicit Range(R const& range)
			: m_IterBegin(std::begin(range)), m_IterEnd(std::end(range))
		{
			//assert(size() >= 0);
		}

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

		constexpr size_type size() const
		{
			return static_cast<size_type>(std::distance(m_IterBegin, m_IterEnd));
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
		Iter m_IterBegin, m_IterEnd;
	};

	namespace detail_
	{
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
				: m_Iter(std::move(iter))
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
	}

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
		return Range<detail_::natRange_ptrIterator<decltype(std::begin(r))>>(r);
	}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	不可复制类
	///	@remark	通过继承此类使子类无法复制
	///	@note	子类仍可被移动
	////////////////////////////////////////////////////////////////////////////////
	class noncopyable
	{
	public:
		noncopyable() = default;

		noncopyable(noncopyable const&) = delete;
		noncopyable(noncopyable &&) = default;
		noncopyable& operator=(noncopyable const&) = delete;
		noncopyable& operator=(noncopyable &&) = default;

	protected:
		~noncopyable() = default;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	不可移动类
	///	@remark	通过继承此类使子类无法复制或移动
	////////////////////////////////////////////////////////////////////////////////
	class nonmovable
	{
	public:
		nonmovable() = default;

		nonmovable(nonmovable &&) = delete;
		nonmovable& operator=(nonmovable &&) = delete;

	protected:
		~nonmovable() = default;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	延迟求值
	////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	class Lazy
	{
	public:
		Lazy()
		{
		}

		explicit Lazy(std::function<T()> function)
			: m_Function{ std::move(function) }
		{
		}

		void ExplicitGenerateValue()
		{
			if (!m_Value)
			{
				if (m_Function)
				{
					m_Value.emplace(m_Function());
				}
				else
				{
					m_Value.emplace();
				}
			}
		}

		nBool HasGenerated() const noexcept
		{
			return m_Value;
		}

		T& GetValue()
		{
			ExplicitGenerateValue();

			return m_Value.value();
		}

		T const& GetValue() const
		{
			// const的Lazy不能生成值，但是如果值已经生成可以获取
			// 如果值未准备好会抛出异常
			return m_Value.value();
		}

	private:
		std::function<T()> m_Function;
		Optional<T> m_Value;
	};

	namespace detail_
	{
		template <typename F, typename ...G>
		struct OverloadType
			: OverloadType<F>::type, OverloadType<G...>::type
		{
			using type = OverloadType;
			using OverloadType<F>::type::operator();
			using OverloadType<G...>::type::operator();

			template <typename F_, typename ...G_>
			constexpr explicit OverloadType(F_&& f, G_&&... g)
				: OverloadType<F>::type(std::forward<F_>(f))
				, OverloadType<G...>::type(std::forward<G_>(g)...)
			{
			}
		};

		template <typename F>
		struct OverloadType<F>
		{
			using type = F;
		};

		template <typename R, typename ...Args>
		struct OverloadType<R(*)(Args...)>
		{
			using type = OverloadType;
			R(* const FunctionPointer)(Args...);

			explicit constexpr OverloadType(R(*fp)(Args...)) noexcept
				: FunctionPointer(fp)
			{
			}

			constexpr R operator()(Args... args) const noexcept(noexcept(FunctionPointer(static_cast<Args&&>(args)...)))
			{
				return FunctionPointer(std::forward<Args>(args)...);
			}
		};
	}

	template <typename... F, typename Overload = typename detail_::OverloadType<std::decay_t<F>...>::type>
	constexpr Overload MakeOverload(F&&... f)
	{
		return Overload(std::forward<F>(f)...);
	}
}

namespace std
{
	template <typename T>
	struct hash<NatsuLib::Optional<T>>
	{
		size_t operator()(NatsuLib::Optional<T> const& keyval) const
		{
			return keyval ? hash<T>{}(*keyval) : 0;
		}
	};
}
