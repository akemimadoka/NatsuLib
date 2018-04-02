////////////////////////////////////////////////////////////////////////////////
///	@file	natLinq.h
///	@brief	语言集成查询
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natMisc.h"
#include "natException.h"
#include <memory>
#include <utility>
#include <vector>
#include <set>
#include <map>
#include <optional>

#ifdef _MSC_VER
#	pragma push_macro("max")
#	pragma push_macro("min")
#endif

#undef max
#undef min

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	允许 Linq 返回值而不是引用的辅助类模板
	///	@tparam	T	要返回的值的类型
	///	@remark	用于 Linq 的模板参数
	////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	struct Valued
	{
		explicit Valued() = delete;
	};

	namespace detail_
	{
		template <typename T, typename OriginT>
		struct IsValuedImpl
			: std::false_type
		{
			using InnerType = OriginT;
		};

		template <typename T, typename U>
		struct IsValuedImpl<Valued<T>, U>
			: std::true_type
		{
			using InnerType = T;
		};

		template <typename T>
		struct IsValued
			: IsValuedImpl<std::remove_cv_t<std::remove_reference_t<T>>, T>
		{
		};

		struct EndIteratorTagType
		{
			constexpr EndIteratorTagType() = default;
		};

		constexpr EndIteratorTagType EndIteratorTag{};

		template <typename Base, typename T>
		using Max = std::conditional_t<std::is_base_of_v<Base, T>, T, Base>;

		template <typename Base, typename T>
		using Min = std::conditional_t<std::is_base_of_v<Base, T>, Base, T>;

		template <typename T, typename U = T, typename = void>
		struct Addable
			: std::false_type
		{
		};

		template <typename T, typename U>
		struct Addable<T, U, std::void_t<decltype(std::declval<T>() + std::declval<U>())>>
			: std::true_type
		{
		};

		template <typename T, typename U = T, typename = void>
		struct Subable
			: std::false_type
		{
		};

		template <typename T, typename U>
		struct Subable<T, U, std::void_t<decltype(std::declval<T>() - std::declval<U>())>>
			: std::true_type
		{
		};

		template <typename T, typename U = T, typename = void>
		struct Multipliable
			: std::false_type
		{
		};

		template <typename T, typename U>
		struct Multipliable<T, U, std::void_t<decltype(std::declval<T>() * std::declval<U>())>>
			: std::true_type
		{
		};

		template <typename T, typename U = T, typename = void>
		struct Dividable
			: std::false_type
		{
		};

		template <typename T, typename U>
		struct Dividable<T, U, std::void_t<decltype(std::declval<T>() / std::declval<U>())>>
			: std::true_type
		{
		};

		template <typename T, typename U = T, typename = void>
		struct CanEqual
			: std::false_type
		{
		};

		template <typename T, typename U>
		struct CanEqual<T, U, std::void_t<std::enable_if_t<std::is_same<decltype(std::declval<T>() == std::declval<U>()), bool>::value>>>
			: std::true_type
		{
		};

		template <typename T, typename U = T, typename = void>
		struct CanNotEqual
			: std::false_type
		{
		};

		template <typename T, typename U>
		struct CanNotEqual<T, U, std::void_t<std::enable_if_t<std::is_same<decltype(std::declval<T>() != std::declval<U>()), bool>::value>>>
			: std::true_type
		{
		};

		template <typename T, typename U = T, typename = void>
		struct CanGreater
			: std::false_type
		{
		};

		template <typename T, typename U>
		struct CanGreater<T, U, std::void_t<std::enable_if_t<std::is_same<decltype(std::declval<T>() > std::declval<U>()), bool>::value>>>
			: std::true_type
		{
		};

		template <typename T, typename U = T, typename = void>
		struct CanLesser
			: std::false_type
		{
		};

		template <typename T, typename U>
		struct CanLesser<T, U, std::void_t<std::enable_if_t<std::is_same<decltype(std::declval<T>() < std::declval<U>()), bool>::value>>>
			: std::true_type
		{
		};

		template <typename T, typename U = T, typename = void>
		struct CanNotLesser
			: std::false_type
		{
		};

		template <typename T, typename U>
		struct CanNotLesser<T, U, std::void_t<std::enable_if_t<std::is_same<decltype(std::declval<T>() >= std::declval<U>()), bool>::value>>>
			: std::true_type
		{
		};

		template <typename T, typename U = T, typename = void>
		struct CanNotGreater
			: std::false_type
		{
		};

		template <typename T, typename U>
		struct CanNotGreater<T, U, std::void_t<std::enable_if_t<std::is_same<decltype(std::declval<T>() <= std::declval<U>()), bool>::value>>>
			: std::true_type
		{
		};

		// 以 EndIterator 为比较目标来判定是否到达迭代结尾
		template <typename Iter>
		class EndIterator
		{
		public:
			typedef typename std::iterator_traits<Iter>::iterator_category iterator_category;
			typedef typename std::iterator_traits<Iter>::value_type value_type;
			typedef typename std::iterator_traits<Iter>::difference_type difference_type;
			typedef typename std::iterator_traits<Iter>::reference reference;
			typedef typename std::iterator_traits<Iter>::pointer pointer;

			constexpr explicit EndIterator(Iter end) noexcept(std::is_nothrow_move_constructible_v<Iter>)
				: m_End{ std::move(end) }
			{
			}

			[[noreturn]] EndIterator& operator++() &
			{
				nat_Throw(natErrException, NatErr_NotSupport, "Try to iterate an end iterator."_nv);
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			[[noreturn]] EndIterator& operator+=(difference_type) &
			{
				nat_Throw(natErrException, NatErr_NotSupport, "Try to iterate an end iterator."_nv);
			}

			[[noreturn]] reference operator*() const
			{
				nat_Throw(natErrException, NatErr_NotSupport, "Try to deref an end iterator."_nv);
			}

		private:
			Iter m_End;

		public:
			// 不应该作为比较依据，应由要比较的迭代器实现
			// 或者提供统一接口让 EndIterator 来比较，这样可以防止反向比较时出现问题
			nBool operator==(EndIterator const& other) const noexcept(noexcept(m_End == other.m_End))
			{
				return m_End == other.m_End;
			}

			nBool operator!=(EndIterator const& other) const noexcept(noexcept(std::declval<EndIterator>() == other))
			{
				return !(*this == other);
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			difference_type operator-(EndIterator const& other) const noexcept(noexcept(m_End - other.m_End))
			{
				return m_End - other.m_End;
			}
		};

		template <typename T, typename Category = std::input_iterator_tag>
		class CommonIterator final
		{
		public:
			typedef Category iterator_category;
			typedef typename IsValued<T>::InnerType value_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::conditional_t<IsValued<T>::value, value_type, std::add_lvalue_reference_t<value_type>> reference;
			typedef std::add_pointer_t<value_type> pointer;

		private:
			// TODO: 针对可能的不同 Category 提供不同的接口
			struct IteratorInterface
			{
				virtual ~IteratorInterface() = default;

				virtual std::unique_ptr<IteratorInterface> Clone() const = 0;
				virtual void MoveNext() = 0;
				virtual reference Deref() const = 0;
				virtual nBool Equals(IteratorInterface const& other) const = 0;
			};

			template <typename Iter_t>
			class IteratorImpl final : public IteratorInterface
			{
				Iter_t m_Iterator;
			public:
				explicit IteratorImpl(Iter_t iterator)
					: m_Iterator(std::move(iterator))
				{
				}

				std::unique_ptr<IteratorInterface> Clone() const override
				{
					return std::make_unique<IteratorImpl>(m_Iterator);
				}

				void MoveNext() override
				{
					++m_Iterator;
				}

				reference Deref() const override
				{
					return *m_Iterator;
				}

				nBool Equals(IteratorInterface const& other) const override
				{
					if (const auto realOther = dynamic_cast<IteratorImpl const*>(std::addressof(other)))
					{
						return m_Iterator == realOther->m_Iterator;
					}

					return false;
				}

				Iter_t GetIterator() const noexcept
				{
					return m_Iterator;
				}
			};

			typedef CommonIterator Self_t;

			std::unique_ptr<IteratorInterface> m_Iterator;

		public:
			// 要包装的迭代器的 difference_type 不能比 std::ptrdiff_t 还大，否则可能出现问题
			template <typename Iter_t, std::enable_if_t<sizeof(typename std::iterator_traits<Iter_t>::difference_type) <= sizeof(difference_type), int> = 0>
			CommonIterator(Iter_t iterator)
				: m_Iterator(std::make_unique<IteratorImpl<Iter_t>>(std::move(iterator)))
			{
			}

			CommonIterator(Self_t const& other)
				: m_Iterator(other.m_Iterator->Clone())
			{
			}

			CommonIterator(Self_t&& other) noexcept
				: m_Iterator(std::move(other.m_Iterator))
			{
			}

			Self_t& operator=(Self_t const& other)
			{
				m_Iterator = other.m_Iterator->Clone();
				return *this;
			}

			Self_t& operator=(Self_t&& other) noexcept
			{
				m_Iterator = std::move(other.m_Iterator);
				return *this;
			}

			template <typename Iter_t>
			Iter_t GetOriginalIterator() const
			{
				auto iter = dynamic_cast<IteratorImpl<Iter_t>*>(m_Iterator.get());
				if (iter)
				{
					return iter->GetIterator();
				}

				nat_Throw(natErrException, NatErr_InvalidArg, "Original iterator is not a Iter_t.");
			}

			Self_t& operator++()
			{
				m_Iterator->MoveNext();

				return *this;
			}

			decltype(auto) operator*() const
			{
				return m_Iterator->Deref();
			}

			nBool operator==(CommonIterator const& other) const
			{
				return m_Iterator->Equals(*other.m_Iterator);
			}

			nBool operator!=(CommonIterator const& other) const
			{
				return !(*this == other);
			}
		};

		template <typename Container_t>
		class StorageIterator final
		{
			typedef StorageIterator<Container_t> Self_t;
			typedef decltype(std::begin(std::declval<Container_t>())) IteratorType;

			std::shared_ptr<Container_t> m_pContainer;
			IteratorType m_Iterator;

		public:
			typedef typename std::iterator_traits<IteratorType>::iterator_category iterator_category;
			typedef typename std::iterator_traits<IteratorType>::value_type value_type;
			typedef typename std::iterator_traits<IteratorType>::difference_type difference_type;
			typedef typename std::iterator_traits<IteratorType>::reference reference;
			typedef typename std::iterator_traits<IteratorType>::pointer pointer;

			StorageIterator(std::shared_ptr<Container_t> const& pContainer, IteratorType const& iterator)
				: m_pContainer(pContainer), m_Iterator(iterator)
			{
			}

			Self_t& operator++() &
			{
				++m_Iterator;
				return *this;
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			Self_t& operator+=(difference_type diff) & noexcept(noexcept(m_Iterator += diff))
			{
				m_Iterator += diff;
				return *this;
			}

			decltype(auto) operator*() const
			{
				return *m_Iterator;
			}

			nBool operator==(StorageIterator const& other) const
			{
				return m_pContainer == other.m_pContainer && m_Iterator == other.m_Iterator;
			}

			nBool operator!=(StorageIterator const& other) const
			{
				return !(*this == other);
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			difference_type operator-(Self_t const& other) const noexcept(noexcept(m_Iterator - other.m_Iterator))
			{
				return m_Iterator - other.m_Iterator;
			}
		};

		template <typename T>
		class EmptyIterator final
		{
			typedef EmptyIterator<T> Self_t;
		public:
			typedef std::input_iterator_tag iterator_category;
			typedef std::remove_cv_t<std::remove_reference_t<T>> value_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::add_lvalue_reference_t<value_type> reference;
			typedef std::add_pointer_t<value_type> pointer;

			Self_t& operator++() &
			{
				return *this;
			}

			[[noreturn]] T& operator*() const
			{
				nat_Throw(natException, "Try to deref an empty iterator."_nv);
			}

			nBool operator==(Self_t const&) const
			{
				return true;
			}

			nBool operator!=(Self_t const&) const
			{
				return false;
			}
		};

		// TODO: 尝试从 CallableObj_t 的类型入手使 end 迭代器不需要拥有 callableObj，需要将 LinqEnumerable 改造成 begin end 迭代器可以是不同类型的
		template <typename Iter_t, typename CallableObj_t>
		class SelectIterator final
		{
			typedef SelectIterator<Iter_t, CallableObj_t> Self_t;

			Iter_t m_Iterator;
			std::optional<CallableObj_t> m_CallableObj;

		public:
			typedef typename std::iterator_traits<Iter_t>::iterator_category iterator_category;
			typedef std::invoke_result_t<CallableObj_t, typename std::iterator_traits<Iter_t>::reference> value_type;
			typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
			typedef value_type reference;
			typedef std::add_pointer_t<value_type> pointer;

			SelectIterator(Iter_t iterator, CallableObj_t callableObj) noexcept(std::is_nothrow_move_constructible_v<Iter_t> && std::is_nothrow_move_constructible_v<CallableObj_t>)
				: m_Iterator(std::move(iterator)), m_CallableObj(std::move(callableObj))
			{
			}

			SelectIterator(Iter_t iterator, EndIteratorTagType) noexcept(std::is_nothrow_move_constructible_v<Iter_t>)
				: m_Iterator(std::move(iterator))
			{
			}

			Self_t& operator++() & noexcept(noexcept(++m_Iterator))
			{
				++m_Iterator;
				return *this;
			}

			// 因为解引用 end 迭代器是 ub，所以不判断是否有 value
			decltype(auto) operator*() const noexcept(noexcept(m_CallableObj.value()(*m_Iterator)))
			{
				return m_CallableObj.value()(*m_Iterator);
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			Self_t& operator+=(difference_type diff) & noexcept(noexcept(m_Iterator += diff))
			{
				m_Iterator += diff;
				return *this;
			}

			nBool operator==(Self_t const& other) const noexcept(noexcept(m_Iterator == other.m_Iterator))
			{
				return m_Iterator == other.m_Iterator/* && m_CallableObj == other.m_CallableObj*/;
			}

			nBool operator!=(Self_t const& other) const noexcept(noexcept(std::declval<Self_t>() == other))
			{
				return !(*this == other);
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			difference_type operator-(Self_t const& other) const noexcept(noexcept(m_Iterator - other.m_Iterator))
			{
				return m_Iterator - other.m_Iterator;
			}
		};

		template <typename Iter_t, typename CallableObj_t>
		class WhereIterator final
		{
			typedef WhereIterator<Iter_t, CallableObj_t> Self_t;

			Iter_t m_Iterator, m_End;
			std::optional<CallableObj_t> m_CallableObj;
		public:
			typedef Min<std::forward_iterator_tag, typename std::iterator_traits<Iter_t>::iterator_category> iterator_category;
			typedef typename std::iterator_traits<Iter_t>::value_type value_type;
			typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
			typedef typename std::iterator_traits<Iter_t>::reference reference;
			typedef typename std::iterator_traits<Iter_t>::pointer pointer;

			WhereIterator(Iter_t iterator, Iter_t end, CallableObj_t callableObj)
				noexcept(std::is_nothrow_move_constructible_v<Iter_t> &&
					std::is_nothrow_move_constructible_v<CallableObj_t> &&
					noexcept(m_Iterator != m_End && !callableObj(*m_Iterator)) &&
					noexcept(++m_Iterator))
				: m_Iterator(std::move(iterator)), m_End(std::move(end)), m_CallableObj(std::move(callableObj))
			{
				auto&& cond = m_CallableObj.value();
				while (m_Iterator != m_End && !cond(*m_Iterator))
				{
					++m_Iterator;
				}
			}

			explicit WhereIterator(Iter_t end)
				: m_Iterator(end), m_End(std::move(end))
			{
			}

			Self_t& operator++() & noexcept(
				noexcept(m_Iterator != m_End && !m_CallableObj.value()(*m_Iterator)) &&
				noexcept(++m_Iterator))
			{
				++m_Iterator;
				auto&& cond = m_CallableObj.value();
				while (m_Iterator != m_End && !cond(*m_Iterator))
				{
					++m_Iterator;
				}

				return *this;
			}

			decltype(auto) operator*() const noexcept(noexcept(*m_Iterator))
			{
				return *m_Iterator;
			}

			nBool operator==(Self_t const& other) const noexcept(noexcept(m_Iterator == other.m_Iterator))
			{
				return m_Iterator == other.m_Iterator && m_End == other.m_End/* && m_CallableObj == other.m_CallableObj*/;
			}

			nBool operator!=(Self_t const& other) const noexcept(noexcept(std::declval<Self_t>() == other))
			{
				return !(*this == other);
			}
		};

		template <typename Iter_t>
		class SkipWhileIterator final
		{
			typedef SkipWhileIterator<Iter_t> Self_t;

			Iter_t m_Iterator;

		public:
			typedef typename std::iterator_traits<Iter_t>::iterator_category iterator_category;
			typedef typename std::iterator_traits<Iter_t>::value_type value_type;
			typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
			typedef typename std::iterator_traits<Iter_t>::reference reference;
			typedef typename std::iterator_traits<Iter_t>::pointer pointer;

			template <typename CallableObj_t>
			SkipWhileIterator(Iter_t iterator, Iter_t const& end, CallableObj_t&& callableObj)
				noexcept(std::is_nothrow_move_constructible_v<Iter_t> &&
					noexcept(m_Iterator != end && std::forward<CallableObj_t>(callableObj)(*m_Iterator)) &&
					noexcept(++m_Iterator))
				: m_Iterator(std::move(iterator))
			{
				while (m_Iterator != end && std::forward<CallableObj_t>(callableObj)(*m_Iterator))
				{
					++m_Iterator;
				}
			}

			explicit SkipWhileIterator(Iter_t end) noexcept(std::is_nothrow_move_constructible_v<Iter_t>)
				: m_Iterator(std::move(end))
			{
			}

			Self_t& operator++() & noexcept(noexcept(++m_Iterator))
			{
				++m_Iterator;
				return *this;
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			Self_t& operator+=(difference_type diff) & noexcept(noexcept(m_Iterator += diff))
			{
				m_Iterator += diff;
				return *this;
			}

			decltype(auto) operator*() const noexcept(noexcept(*m_Iterator))
			{
				return *m_Iterator;
			}

			nBool operator==(Self_t const& other) const noexcept(noexcept(m_Iterator == other.m_Iterator))
			{
				return m_Iterator == other.m_Iterator;
			}

			nBool operator!=(Self_t const& other) const noexcept(noexcept(std::declval<Self_t>() == other))
			{
				return !(*this == other);
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			difference_type operator-(Self_t const& other) const noexcept(noexcept(m_Iterator - other.m_Iterator))
			{
				return m_Iterator - other.m_Iterator;
			}
		};

		template <typename Iter_t>
		class TakeIterator final
		{
			typedef TakeIterator<Iter_t> Self_t;
		public:
			typedef typename std::iterator_traits<Iter_t>::iterator_category iterator_category;
			typedef typename std::iterator_traits<Iter_t>::value_type value_type;
			typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
			typedef typename std::iterator_traits<Iter_t>::reference reference;
			typedef typename std::iterator_traits<Iter_t>::pointer pointer;

			typedef std::make_unsigned_t<difference_type> size_type;

		private:
			Iter_t m_Iterator, m_End;
			size_type m_TakeCount;

		public:
			TakeIterator(Iter_t const& iterator, Iter_t const& end, size_type count) noexcept(std::is_nothrow_copy_constructible_v<Iter_t>)
				: m_Iterator(count ? iterator : end), m_End(end), m_TakeCount{ count }
			{
			}

			Self_t& operator++() & noexcept(noexcept(++m_Iterator) && noexcept(m_Iterator = m_End))
			{
				if (!m_TakeCount)
				{
					m_Iterator = m_End;
				}
				else
				{
					++m_Iterator;
					--m_TakeCount;
				}

				return *this;
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			Self_t& operator+=(difference_type diff) & noexcept(noexcept(m_Iterator = m_End) && noexcept(m_Iterator += diff))
			{
				if (m_TakeCount < diff)
				{
					m_Iterator = m_End;
					m_TakeCount = 0;
				}
				else
				{
					m_Iterator += diff;
					m_TakeCount -= diff;
				}

				return *this;
			}

			decltype(auto) operator*() const noexcept(noexcept(*m_Iterator))
			{
				return *m_Iterator;
			}

			nBool operator==(Self_t const& other) const noexcept(noexcept(m_Iterator == other.m_Iterator))
			{
				return m_Iterator == other.m_Iterator && m_End == other.m_End;
			}

			nBool operator!=(Self_t const& other) const noexcept(noexcept(std::declval<Self_t>() == other))
			{
				return !(*this == other);
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			difference_type operator-(Self_t const& other) const noexcept(noexcept(m_Iterator - other.m_Iterator))
			{
				return m_Iterator - other.m_Iterator;
			}
		};

		template <typename Iter_t, typename CallableObj_t>
		class TakeWhileIterator final
		{
			typedef TakeWhileIterator<Iter_t, CallableObj_t> Self_t;

			Iter_t m_Iterator, m_End;
			std::optional<CallableObj_t> m_CallableObj;
		public:
			typedef Min<std::forward_iterator_tag, typename std::iterator_traits<Iter_t>::iterator_category> iterator_category;
			typedef typename std::iterator_traits<Iter_t>::value_type value_type;
			typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
			typedef typename std::iterator_traits<Iter_t>::reference reference;
			typedef typename std::iterator_traits<Iter_t>::pointer pointer;

			TakeWhileIterator(Iter_t iterator, Iter_t end, CallableObj_t callableObj)
				noexcept(std::is_nothrow_move_constructible_v<Iter_t> && std::is_nothrow_move_constructible_v<CallableObj_t> &&
					noexcept(m_Iterator != m_End && !callableObj(*m_Iterator)) && noexcept(m_Iterator = m_End))
				: m_Iterator(std::move(iterator)), m_End(std::move(end)), m_CallableObj(std::move(callableObj))
			{
				auto&& cond = m_CallableObj.value();
				if (m_Iterator != m_End && !cond(*m_Iterator))
				{
					m_Iterator = m_End;
				}
			}

			explicit TakeWhileIterator(Iter_t end) noexcept(std::is_nothrow_move_constructible_v<Iter_t>)
				: m_Iterator(end), m_End(std::move(end))
			{
			}

			Self_t& operator++() & noexcept(noexcept(!m_CallableObj.value()(*++m_Iterator)) && noexcept(m_Iterator = m_End))
			{
				auto&& cond = m_CallableObj.value();
				if (!cond(*++m_Iterator))
				{
					m_Iterator = m_End;
				}

				return *this;
			}

			decltype(auto) operator*() const noexcept(noexcept(*m_Iterator))
			{
				return *m_Iterator;
			}

			nBool operator==(Self_t const& other) const noexcept(noexcept(m_Iterator == other.m_Iterator))
			{
				return m_Iterator == other.m_Iterator && m_End == other.m_End;
			}

			nBool operator!=(Self_t const& other) const noexcept(noexcept(std::declval<Self_t>() == other))
			{
				return !(*this == other);
			}
		};

		template <typename Iter1_t, typename Iter2_t>
		class ConcatIterator final
		{
			typedef ConcatIterator<Iter1_t, Iter2_t> Self_t;

			Iter1_t m_Current1, m_End1;
			Iter2_t m_Current2, m_End2;
			
		public:
			typedef typename std::iterator_traits<Iter1_t>::iterator_category iterator_category1;
			typedef typename std::iterator_traits<Iter1_t>::value_type value_type1;
			typedef typename std::iterator_traits<Iter1_t>::difference_type difference_type1;
			typedef typename std::iterator_traits<Iter1_t>::reference reference1;
			typedef typename std::iterator_traits<Iter1_t>::pointer pointer1;

			typedef typename std::iterator_traits<Iter2_t>::iterator_category iterator_category2;
			typedef typename std::iterator_traits<Iter2_t>::value_type value_type2;
			typedef typename std::iterator_traits<Iter2_t>::difference_type difference_type2;
			typedef typename std::iterator_traits<Iter2_t>::reference reference2;
			typedef typename std::iterator_traits<Iter2_t>::pointer pointer2;

			typedef Min<iterator_category1, iterator_category2> iterator_category;
			typedef std::common_type_t<value_type1, value_type2> value_type;
			typedef std::common_type_t<difference_type1, difference_type2> difference_type;
			typedef std::common_type_t<reference1, reference2> reference;
			typedef std::common_type_t<pointer1, pointer2> pointer;

			ConcatIterator(Iter1_t current1, Iter1_t end1, Iter2_t current2, Iter2_t end2) noexcept(std::is_nothrow_move_constructible_v<Iter1_t> && std::is_nothrow_move_constructible_v<Iter2_t>)
				: m_Current1(std::move(current1)), m_End1(std::move(end1)), m_Current2(std::move(current2)), m_End2(std::move(end2))
			{
			}

			Self_t& operator++() & noexcept(noexcept(m_Current1 != m_End1) && noexcept(++m_Current1) && noexcept(++m_Current2))
			{
				if (m_Current1 != m_End1)
				{
					++m_Current1;
				}
				else
				{
					++m_Current2;
				}

				return *this;
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			Self_t& operator+=(difference_type diff) & noexcept(
				noexcept(m_End1 - m_Current1) &&
				noexcept(m_Current1 += diff) &&
				noexcept(m_Current1 += std::declval<difference_type1>()) &&
				noexcept(m_Current2 += diff - std::declval<difference_type1>()))
			{
				const auto diff1 = m_End1 - m_Current1;
				if (diff1 < diff)
				{
					m_Current1 += diff1;
					m_Current2 += diff - diff1;
				}
				else
				{
					m_Current1 += diff;
				}

				return *this;
			}

			decltype(auto) operator*() const noexcept(noexcept(m_Current1 != m_End1 ? *m_Current1 : *m_Current2))
			{
				return m_Current1 != m_End1 ? *m_Current1 : *m_Current2;
			}

			nBool operator==(Self_t const& other) const noexcept(noexcept(m_Current1 == other.m_Current1 && m_Current2 == other.m_Current2))
			{
				return m_Current1 == other.m_Current1 && m_End1 == other.m_End1 && m_Current2 == other.m_Current2 && m_End2 == other.m_End2;
			}

			nBool operator!=(Self_t const& other) const noexcept(noexcept(std::declval<Self_t>() == other))
			{
				return !(*this == other);
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			difference_type operator-(Self_t const& other) const noexcept(noexcept(m_Current1 == m_End1 ? m_Current2 - other.m_Current2 : m_Current1 - other.m_Current1))
			{
				return m_Current1 == m_End1 ? m_Current2 - other.m_Current2 : m_Current1 - other.m_Current1;
			}
		};

		template <typename Iter_t, typename = void>
		class ReverseIterator final
		{
		public:
			typedef std::vector<typename std::iterator_traits<Iter_t>::value_type> BufferVector;
			typedef typename BufferVector::reverse_iterator RealIter;

			typedef typename std::iterator_traits<RealIter>::iterator_category iterator_category;
			typedef typename std::iterator_traits<RealIter>::value_type value_type;
			typedef typename std::iterator_traits<RealIter>::difference_type difference_type;
			typedef typename std::iterator_traits<RealIter>::reference reference;
			typedef typename std::iterator_traits<RealIter>::pointer pointer;

		private:
			typedef ReverseIterator<Iter_t> Self_t;

			BufferVector m_Buffer;
			Iter_t m_OriginBegin, m_OriginEnd;
			RealIter m_Current, m_End;
			
		public:
			ReverseIterator(Iter_t begin, Iter_t end) noexcept(std::is_nothrow_constructible_v<BufferVector, Iter_t, Iter_t> && std::is_nothrow_move_constructible_v<Iter_t>)
				: m_Buffer(begin, end), m_OriginBegin(std::move(begin)), m_OriginEnd(std::move(end)), m_Current(std::rbegin(m_Buffer)), m_End(std::rend(m_Buffer))
			{
			}

			Self_t& operator++() & noexcept(noexcept(++m_Current))
			{
				++m_Current;
				return *this;
			}

			decltype(auto) operator*() const noexcept(noexcept(*m_Current))
			{
				return *m_Current;
			}

			nBool operator==(Self_t const& other) const noexcept(noexcept(m_OriginBegin == other.m_OriginBegin && std::distance(m_Current, m_End)))
			{
				return m_OriginBegin == other.m_OriginBegin &&
					m_OriginEnd == other.m_OriginEnd &&
					std::distance(m_Current, m_End) == std::distance(other.m_Current, other.m_End);
			}

			nBool operator!=(Self_t const& other) const noexcept(noexcept(std::declval<Self_t>() == other))
			{
				return !(*this == other);
			}
		};

		template <typename Iter_t>
		class ReverseIterator<Iter_t,
			std::enable_if_t<
				std::is_base_of<
					std::bidirectional_iterator_tag,
					typename std::iterator_traits<Iter_t>::iterator_category>::value>>
			final
		{
			typedef ReverseIterator<Iter_t> Self_t;

			typedef std::reverse_iterator<Iter_t> RealIter;

			RealIter m_Current;

		public:
			typedef typename std::iterator_traits<RealIter>::iterator_category iterator_category;
			typedef typename std::iterator_traits<RealIter>::value_type value_type;
			typedef typename std::iterator_traits<RealIter>::difference_type difference_type;
			typedef typename std::iterator_traits<RealIter>::reference reference;
			typedef typename std::iterator_traits<RealIter>::pointer pointer;

			// 与另一形式保持一致，但不使用第二个参数
			ReverseIterator(Iter_t const& cur, Iter_t const&) noexcept(std::is_nothrow_constructible_v<RealIter, Iter_t const&>)
				: m_Current{ cur }
			{
			}

			Self_t& operator++() & noexcept(noexcept(++m_Current))
			{
				++m_Current;
				return *this;
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			Self_t& operator+=(difference_type diff) & noexcept(noexcept(m_Current += diff))
			{
				m_Current += diff;
				return *this;
			}

			decltype(auto) operator*() const noexcept(noexcept(*m_Current))
			{
				return *m_Current;
			}

			nBool operator==(Self_t const& other) const noexcept(noexcept(m_Current == other.m_Current))
			{
				return m_Current == other.m_Current;
			}

			nBool operator!=(Self_t const& other) const noexcept(noexcept(std::declval<Self_t>() == other))
			{
				return !(*this == other);
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			difference_type operator-(Self_t const& other) const noexcept(noexcept(m_Current - other.m_Current))
			{
				return m_Current - other.m_Current;
			}
		};

		template <typename Iter1_t, typename Iter2_t>
		class ZipIterator final
		{
			typedef ZipIterator<Iter1_t, Iter2_t> Self_t;

			Iter1_t m_Current1, m_End1;
			Iter2_t m_Current2, m_End2;
		public:
			typedef typename std::iterator_traits<Iter1_t>::iterator_category iterator_category1;
			typedef typename std::iterator_traits<Iter1_t>::value_type value_type1;
			typedef typename std::iterator_traits<Iter1_t>::difference_type difference_type1;
			typedef typename std::iterator_traits<Iter1_t>::reference reference1;
			typedef typename std::iterator_traits<Iter1_t>::pointer pointer1;

			typedef typename std::iterator_traits<Iter2_t>::iterator_category iterator_category2;
			typedef typename std::iterator_traits<Iter2_t>::value_type value_type2;
			typedef typename std::iterator_traits<Iter2_t>::difference_type difference_type2;
			typedef typename std::iterator_traits<Iter2_t>::reference reference2;
			typedef typename std::iterator_traits<Iter2_t>::pointer pointer2;

			typedef std::pair<reference1, reference2> Element_t;

			typedef Min<iterator_category1, iterator_category2> iterator_category;
			typedef Element_t value_type;
			typedef std::common_type_t<difference_type1, difference_type2> difference_type;
			typedef Element_t reference;
			typedef Element_t* pointer;

			ZipIterator(Iter1_t current1, Iter1_t end1, Iter2_t current2, Iter2_t end2) noexcept(std::is_nothrow_move_constructible_v<Iter1_t> && std::is_nothrow_move_constructible_v<Iter2_t>)
				: m_Current1(std::move(current1)), m_End1(std::move(end1)), m_Current2(std::move(current2)), m_End2(std::move(end2))
			{
			}

			// TODO: 类似这样可能同时对两个对象进行操作无法实现强异常安全
			Self_t& operator++() & noexcept(noexcept(m_Current1 != m_End1) && noexcept(++m_Current1) && noexcept(m_Current2 != m_End2) && noexcept(++m_Current2))
			{
				if (m_Current1 != m_End1)
				{
					++m_Current1;
				}

				if (m_Current2 != m_End2)
				{
					++m_Current2;
				}

				return *this;
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			Self_t& operator+=(difference_type diff) & noexcept(noexcept(m_Current1 += diff) && noexcept(m_Current2 += diff))
			{
				m_Current1 += diff;
				m_Current2 += diff;

				return *this;
			}

			Element_t operator*() const noexcept(std::is_nothrow_constructible_v<Element_t, decltype(*m_Current1), decltype(*m_Current2)>)
			{
				return Element_t(*m_Current1, *m_Current2);
			}

			nBool operator==(Self_t const& other) const noexcept(noexcept(m_Current1 == other.m_Current1 && m_Current2 == other.m_Current2))
			{
				return m_Current1 == other.m_Current1 && m_End1 == other.m_End1 && m_Current2 == other.m_Current2 && m_End2 == other.m_End2;
			}

			nBool operator!=(Self_t const& other) const noexcept(noexcept(std::declval<Self_t>() == other))
			{
				return !(*this == other);
			}

			template <typename IterCate = iterator_category, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, IterCate>, int> = 0>
			difference_type operator-(Self_t const& other) const
				noexcept(noexcept(std::max(static_cast<difference_type>(m_Current1 - other.m_Current1),
					static_cast<difference_type>(m_Current2 - other.m_Current2))))
			{
				return std::max(static_cast<difference_type>(m_Current1 - other.m_Current1),
					static_cast<difference_type>(m_Current2 - other.m_Current2));
			}
		};
	}

	template <typename Iter_t>
	class LinqEnumerable;

	template <typename T>
	class Linq;

	template <typename Container>
	LinqEnumerable<detail_::StorageIterator<Container>> from_container(Container container);

	template <typename T>
	LinqEnumerable<detail_::EmptyIterator<T>> from_empty();

	template <typename Iter_t>
	class LinqEnumerable
	{
		typedef std::decay_t<decltype(*std::declval<Iter_t>())> Element_t;

	public:
		typedef Iter_t iterator;
		typedef typename std::iterator_traits<Iter_t>::value_type value_type;
		typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
		typedef typename std::iterator_traits<Iter_t>::reference reference;
		typedef std::make_unsigned_t<difference_type> size_type;

		typedef LinqEnumerable<Iter_t> Self_t;

	private:
		Range<Iter_t> m_Range;
		mutable Optional<size_t> m_Size;

	public:
		constexpr LinqEnumerable(Iter_t begin, Iter_t end)
			: m_Range(std::move(begin), std::move(end))
		{
		}

		constexpr LinqEnumerable(LinqEnumerable const& other) = default;
		constexpr LinqEnumerable(LinqEnumerable&& other) = default;

		LinqEnumerable& operator=(LinqEnumerable const& other) = default;
		LinqEnumerable& operator=(LinqEnumerable && other) = default;

		constexpr Iter_t begin() const
		{
			return m_Range.begin();
		}

		constexpr Iter_t end() const
		{
			return m_Range.end();
		}

		size_type size() const
		{
			if (!m_Size)
			{
				m_Size.emplace(static_cast<size_t>(std::distance(m_Range.begin(), m_Range.end())));
			}

			return m_Size.value();
		}

		size_type count() const
		{
			return size();
		}

		constexpr nBool empty() const
		{
			return m_Range.empty();
		}

		template <typename CallableObj>
		LinqEnumerable<detail_::SelectIterator<Iter_t, CallableObj>> select(CallableObj&& callableObj) const
		{
			return LinqEnumerable<detail_::SelectIterator<Iter_t, CallableObj>>(detail_::SelectIterator<Iter_t, CallableObj>(m_Range.begin(), std::forward<CallableObj>(callableObj)),
				detail_::SelectIterator<Iter_t, CallableObj>(m_Range.end(), detail_::EndIteratorTag));
		}

		template <typename T>
		auto ofType() const
		{
			return select([](auto&& origin)
			{
				return static_cast<T>(static_cast<std::remove_reference_t<decltype(origin)>&&>(origin));
			});
		}

		template <typename CallableObj>
		LinqEnumerable<detail_::WhereIterator<Iter_t, CallableObj>> where(CallableObj&& callableObj) const
		{
			return LinqEnumerable<detail_::WhereIterator<Iter_t, CallableObj>>(detail_::WhereIterator<Iter_t, CallableObj>(m_Range.begin(), m_Range.end(), std::forward<CallableObj>(callableObj)),
				detail_::WhereIterator<Iter_t, CallableObj>(m_Range.end()));
		}

		LinqEnumerable skip(difference_type count)
		{
			LinqEnumerable ret(*this);
			ret.m_Range.pop_front(count);
			return ret;
		}

		template <typename CallableObj>
		LinqEnumerable<detail_::SkipWhileIterator<Iter_t>> skip_while(CallableObj&& callableObj) const
		{
			return LinqEnumerable<detail_::SkipWhileIterator<Iter_t>>(detail_::SkipWhileIterator<Iter_t>(m_Range.begin(), m_Range.end(), std::forward<CallableObj>(callableObj)),
				detail_::SkipWhileIterator<Iter_t>(m_Range.end()));
		}

		LinqEnumerable<detail_::TakeIterator<Iter_t>> take(difference_type count)
		{
			return LinqEnumerable<detail_::TakeIterator<Iter_t>>(detail_::TakeIterator<Iter_t>(m_Range.begin(), m_Range.end(), count),
				detail_::TakeIterator<Iter_t>(m_Range.end(), m_Range.end(), count));
		}

		template <typename CallableObj>
		LinqEnumerable<detail_::TakeWhileIterator<Iter_t, CallableObj>> take_while(CallableObj&& callableObj) const
		{
			return LinqEnumerable<detail_::TakeWhileIterator<Iter_t, CallableObj>>(detail_::TakeWhileIterator<Iter_t, CallableObj>(m_Range.begin(), m_Range.end(), std::forward<CallableObj>(callableObj)),
				detail_::TakeWhileIterator<Iter_t, CallableObj>(m_Range.end()));
		}

		template <typename Iter2_t>
		LinqEnumerable<detail_::ConcatIterator<Iter_t, Iter2_t>> concat(LinqEnumerable<Iter2_t> const& other) const
		{
			return LinqEnumerable<detail_::ConcatIterator<Iter_t, Iter2_t>>(detail_::ConcatIterator<Iter_t, Iter2_t>(m_Range.begin(), m_Range.end(), other.begin(), other.end()),
				detail_::ConcatIterator<Iter_t, Iter2_t>(m_Range.end(), m_Range.end(), other.end(), other.end()));
		}

		template <typename T>
		nBool Contains(T const& item) const
		{
			return std::find(m_Range.begin(), m_Range.end(), item);
		}

		decltype(auto) element_at(difference_type index)
		{
			return *std::next(m_Range.begin(), index);
		}

		decltype(auto) element_at(difference_type index) const
		{
			return *std::next(m_Range.begin(), index);
		}

		auto distinct() const
		{
			std::set<Element_t> tmpSet(m_Range.begin(), m_Range.end());
			return from_container(std::move(tmpSet));
		}

		template <typename Iter2_t>
		auto except(LinqEnumerable<Iter2_t> const& other) const
		{
			std::set<Element_t> tmpSet(m_Range.begin(), m_Range.end());
			for (auto&& item : other)
			{
				tmpSet.erase(item);
			}
			return from_container(std::move(tmpSet));
		}

		template <typename Iter2_t>
		auto intersect(LinqEnumerable<Iter2_t> const& other) const
		{
			std::set<Element_t> tmpSet(m_Range.begin(), m_Range.end()), tmpSet2(other.begin(), other.end()), result;
			for (auto&& item : tmpSet2)
			{
				auto iter = tmpSet.find(item);
				if (iter != tmpSet.end())
				{
					result.insert(item);
					tmpSet.erase(iter);
				}
			}
			return from_container(std::move(result));
		}

		template <typename Iter2_t>
		auto union_with(LinqEnumerable<Iter2_t> const& other) const
		{
			return concat(other).distinct();
		}

		Element_t aggregate() const
		{
			return sum();
		}

		template <typename CallableObj>
		Element_t aggregate(CallableObj&& callableObj) const
		{
			if constexpr (std::is_default_constructible_v<Element_t>)
			{
				return aggregate(Element_t{}, std::forward<CallableObj>(callableObj));
			}
			else
			{
				if (m_Range.empty())
				{
					nat_Throw(natException, "Range is empty and there is no default constructor for this type."_nv);
				}

				auto iter = m_Range.begin();
				Element_t result = *iter;
				while (++iter != m_Range.end())
				{
					result = std::forward<CallableObj>(callableObj)(std::move(result), *iter);
				}
				return result;
			}
		}

		decltype(auto) first() const
		{
			if (empty())
			{
				nat_Throw(natErrException, NatErr_OutOfRange, "No element."_nv);
			}

			return *begin();
		}

		template <typename CallableObj>
		decltype(auto) first(CallableObj&& callableObj) const
		{
			if (empty())
			{
				nat_Throw(natErrException, NatErr_OutOfRange, "No element."_nv);
			}

			for (auto&& item : *this)
			{
				// 不能转发 item，因为可能移动，若移动则可能返回的是移动后的值
				if (!!std::forward<CallableObj>(callableObj)(item))
				{
					return item;
				}
			}

			nat_Throw(natErrException, NatErr_OutOfRange, "No matching element found."_nv);
		}

		template <typename T>
		auto first_or_default(T&& defItem) const -> std::conditional_t<std::is_convertible_v<decltype(defItem), reference>, reference, value_type>
		{
			return empty() ? std::forward<T>(defItem) : *begin();
		}

		template <typename T, typename CallableObj>
		auto first_or_default(T&& defItem, CallableObj&& callableObj) const -> std::conditional_t<std::is_convertible_v<decltype(defItem), reference>, reference, value_type>
		{
			if (empty())
			{
				return std::forward<T>(defItem);
			}

			for (auto&& item : *this)
			{
				if (!!std::forward<CallableObj>(callableObj)(item))
				{
					return item;
				}
			}

			return std::forward<T>(defItem);
		}

		template <typename Result_t, typename CallableObj>
		Result_t aggregate(Result_t result, CallableObj&& callableObj) const
		{
			for (auto&& item : *this)
			{
				result = std::forward<CallableObj>(callableObj)(std::move(result), item);
			}
			return result;
		}

		template <typename CallableObj>
		nBool all(CallableObj&& callableObj) const
		{
			return select(std::forward<CallableObj>(callableObj)).aggregate(true, [](nBool a, nBool b) { return a && b; });
		}

		// FIXME: 可能是错误的实现
		nBool any() const
		{
			return !empty();
		}

		// FIXME: 可能是错误的实现
		template <typename CallableObj>
		nBool any(CallableObj&& callableObj) const
		{
			return !where(std::forward<CallableObj>(callableObj)).empty();
		}

		template <typename Result_t = Element_t>
		auto average() const
		{
			if constexpr (std::conjunction<detail_::Addable<Result_t, Element_t>, detail_::Dividable<Result_t>>::value)
			{
				Result_t result{};
				return aggregate(result, [](Result_t const& res, Element_t const& item)
				{
					return res + item;
				}) / static_cast<Result_t>(m_Range.size());
			}
			else
			{
				nat_Throw(natException, "Cannot apply such operation to this type."_nv);
			}
		}

		auto max() const
		{
			if constexpr (detail_::CanGreater<Element_t>::value)
			{
				return aggregate([](const Element_t& a, const Element_t& b) { return a > b ? a : b; });
			}
			else
			{
				nat_Throw(natException, "Cannot apply such operation to this type."_nv);
			}
		}

		auto min() const
		{
			if constexpr (detail_::CanLesser<Element_t>::value)
			{
				return aggregate([](const Element_t& a, const Element_t& b) { return a < b ? a : b; });
			}
			else
			{
				nat_Throw(natException, "Cannot apply such operation to this type."_nv);
			}
		}

		auto sum() const
		{
			if constexpr (detail_::Addable<Element_t>::value)
			{
				return aggregate(std::plus<Element_t>{});
			}
			else
			{
				nat_Throw(natException, "Cannot apply such operation to this type."_nv);
			}
		}

		auto product() const
		{
			if constexpr (detail_::Multipliable<Element_t>::value)
			{
				return aggregate(static_cast<Element_t>(1), [](const Element_t& a, const Element_t& b) { return a * b; });
			}
			else
			{
				nat_Throw(natException, "Cannot apply such operation to this type."_nv);
			}
		}

		template <typename CallableObj>
		auto select_many(CallableObj&& callableObj) const
		{
			typedef decltype(std::forward<CallableObj>(callableObj)(std::declval<Element_t>())) Collection_t;
			typedef decltype(*std::begin(std::declval<Collection_t>())) Value_t;
			return select(std::forward<CallableObj>(callableObj)).aggregate(from_empty<Value_t>(), [](Linq<Value_t> const& a, Collection_t const& b)
			{
				return a.concat(b);
			});
		}

		template <typename CallableObj>
		auto group_by(CallableObj&& keySelector) const
		{
			typedef decltype(std::forward<CallableObj>(keySelector)(std::declval<Element_t>())) Key_t;
			std::map<Key_t, std::vector<Element_t>> tmpMap;
			for (auto&& item : *this)
			{
				auto&& key = std::forward<CallableObj>(keySelector)(item);
				tmpMap[key].emplace_back(item);
			}

			std::vector<std::pair<Key_t, Linq<Element_t>>> result;
			for (auto&& item : tmpMap)
			{
				result.emplace_back(std::make_pair<Key_t, Linq<Element_t>>(item.first, from_container(std::move(item.second))));
			}

			return from_container(std::move(result));
		}

		template <typename Iter2_t, typename CallableObj1, typename CallableObj2>
		auto full_join(LinqEnumerable<Iter2_t> const& e, CallableObj1&& keySelector1, CallableObj2&& keySelector2) const
		{
			typedef std::remove_reference_t<decltype(std::forward<CallableObj1>(keySelector1)(std::declval<Element_t>()))> Key1_t;
			typedef std::remove_reference_t<decltype(std::forward<CallableObj2>(keySelector2)(std::declval<Element_t>()))> Key2_t;
			static_assert(std::is_same<Key1_t, Key2_t>::value, "Key1_t and Key2_t should be same.");
			typedef std::common_type_t<Key1_t, Key2_t> Key_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter_t>())>> Value1_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter2_t>())>> Value2_t;
			typedef std::tuple<Key_t, Linq<Element_t>, Linq<Element_t>> FullJoinResult_t;

			std::multimap<Key_t, Value1_t> map1;
			std::multimap<Key_t, Value2_t> map2;

			for (auto&& item : *this)
			{
				map1.insert(std::make_pair(std::forward<CallableObj1>(keySelector1)(item), item));
			}

			for (auto&& item : e)
			{
				map2.insert(std::make_pair(std::forward<CallableObj2>(keySelector2)(item), item));
			}

			std::vector<FullJoinResult_t> result;
			auto lower1 = map1.begin();
			auto lower2 = map2.begin();
			while (lower1 != map1.end() && lower2 != map2.end())
			{
				auto key1 = lower1->first;
				auto key2 = lower2->first;
				auto upper1 = map1.upper_bound(key1);
				auto upper2 = map2.upper_bound(key2);

				if (key1 < key2)
				{
					std::vector<Value1_t> outers;
					outers.reserve(std::distance(lower1, upper1));
					for (auto iter = lower1; iter != upper1; ++iter)
					{
						outers.emplace_back(iter->second);
					}
					result.emplace_back(std::make_tuple(key1, from_container(std::move(outers)), from_empty<Value2_t>()));
					lower1 = upper1;
				}
				else if (key1 > key2)
				{
					std::vector<Value2_t> inners;
					inners.reserve(std::distance(lower2, upper2));
					for (auto iter = lower2; iter != upper2; ++iter)
					{
						inners.emplace_back(iter->second);
					}
					result.emplace_back(std::make_tuple(key2, from_empty<Value1_t>(), from_container(std::move(inners))));
					lower2 = upper2;
				}
				else
				{
					std::vector<Value1_t> outers;
					outers.reserve(std::distance(lower1, upper1));
					for (auto iter = lower1; iter != upper1; ++iter)
					{
						outers.emplace_back(iter->second);
					}
					std::vector<Value2_t> inners;
					inners.reserve(std::distance(lower2, upper2));
					for (auto iter = lower2; iter != upper2; ++iter)
					{
						inners.emplace_back(iter->second);
					}
					result.emplace_back(std::make_tuple(key1, from_container(std::move(outers)), from_container(std::move(inners))));
					lower1 = upper1;
					lower2 = upper2;
				}
			}

			return from_container(std::move(result));
		}

		template <typename Iter2_t, typename CallableObj1, typename CallableObj2>
		auto group_join(LinqEnumerable<Iter2_t> const& e, CallableObj1&& keySelector1, CallableObj2&& keySelector2) const
		{
			typedef std::remove_reference_t<decltype(std::forward<CallableObj1>(keySelector1)(std::declval<Element_t>()))> Key1_t;
			typedef std::remove_reference_t<decltype(std::forward<CallableObj2>(keySelector2)(std::declval<Element_t>()))> Key2_t;
			static_assert(std::is_same<Key1_t, Key2_t>::value, "Key1_t and Key2_t should be same.");
			typedef std::common_type_t<Key1_t, Key2_t> Key_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter_t>())>> Value1_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter2_t>())>> Value2_t;
			typedef std::tuple<Key_t, Linq<Element_t>, Linq<Element_t>> FullJoinResult_t;
			typedef std::tuple<Key_t, Value1_t, Linq<Value2_t>> GroupJoinResult_t;

			return full_join(e, std::forward<CallableObj1>(keySelector1), std::forward<CallableObj2>(keySelector2)).select_many([](const FullJoinResult_t& item)
			{
				Linq<Value1_t> outers = item.second.first;
				return outers.select([&item](const Value1_t& outer)
				{
					return GroupJoinResult_t{ item.first, outer, item.second.second };
				});
			});
		}

		template <typename Iter2_t, typename CallableObj1, typename CallableObj2>
		auto join(LinqEnumerable<Iter2_t> const& e, CallableObj1&& keySelector1, CallableObj2&& keySelector2) const
		{
			typedef std::remove_reference_t<decltype(std::forward<CallableObj1>(keySelector1)(std::declval<Element_t>()))> Key1_t;
			typedef std::remove_reference_t<decltype(std::forward<CallableObj2>(keySelector2)(std::declval<Element_t>()))> Key2_t;
			static_assert(std::is_same<Key1_t, Key2_t>::value, "Key1_t and Key2_t should be same.");
			typedef std::common_type_t<Key1_t, Key2_t> Key_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter_t>())>> Value1_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter2_t>())>> Value2_t;
			typedef std::tuple<Key_t, Value1_t, Linq<Value2_t>> GroupJoinResult_t;
			typedef std::tuple<Key_t, Value1_t, Value2_t> JoinResult_t;

			return group_join(e, std::forward<CallableObj1>(keySelector1), std::forward<CallableObj2>(keySelector2)).select_many([](const GroupJoinResult_t& item)
			{
				Linq<Value2_t> inners = item.second.second;
				return inners.select([&item](const Value2_t& inner)
				{
					return JoinResult_t{ item.first, item.second.first, inner };
				});
			});
		}

		template <typename CallableObj>
		auto first_order_by(CallableObj&& keySelector) const
		{
			typedef std::remove_reference_t<decltype(std::forward<CallableObj>(keySelector)(std::declval<Element_t>()))> Key_t;

			return group_by(std::forward<CallableObj>(keySelector)).select([](std::pair<Key_t, Linq<Element_t>> const& p)
			{
				return p.second;
			});
		}

		template <typename CallableObj>
		auto then_order_by(CallableObj&& keySelector) const
		{
			return select_many([&keySelector](Element_t const& values)
			{
				return values.first_order_by(std::forward<CallableObj>(keySelector));
			});
		}

		template <typename CallableObj>
		auto order_by(CallableObj&& keySelector) const
		{
			return first_order_by(std::forward<CallableObj>(keySelector)).select_many([](const Linq<Element_t>& values) { return values; });
		}

		template <typename Iter2_t>
		auto zip(LinqEnumerable<Iter2_t> const& e) const
		{
			return LinqEnumerable<detail_::ZipIterator<Iter_t, Iter2_t>>(detail_::ZipIterator<Iter_t, Iter2_t>(m_Range.begin(), m_Range.end(), e.begin(), e.end()),
				detail_::ZipIterator<Iter_t, Iter2_t>(m_Range.end(), m_Range.end(), e.end(), e.end()));
		}

		auto reverse() const
		{
			return LinqEnumerable<detail_::ReverseIterator<Iter_t>>(detail_::ReverseIterator<Iter_t>(m_Range.begin(), m_Range.end()),
				detail_::ReverseIterator<Iter_t>(m_Range.begin(), m_Range.begin()));
		}

		template <typename Container>
		Container Cast() const
		{
			return Container{ m_Range.begin(), m_Range.end() };
		}
	};

	template <typename T>
	class Linq final
		: public LinqEnumerable<detail_::CommonIterator<T>>
	{
		typedef LinqEnumerable<detail_::CommonIterator<T>> Base;
	public:
		template <typename Iter_t>
		Linq(LinqEnumerable<Iter_t> const& linqEnumerable)
			: Base(detail_::CommonIterator<T>(linqEnumerable.begin()),
				detail_::CommonIterator<T>(linqEnumerable.end()))
		{
		}
	};

	template <typename Container>
	LinqEnumerable<detail_::StorageIterator<Container>> from_pointertocontainer(std::shared_ptr<Container> const& pContainer)
	{
		return LinqEnumerable<detail_::StorageIterator<Container>>(detail_::StorageIterator<Container>(pContainer, std::begin(*pContainer)),
			detail_::StorageIterator<Container>(pContainer, std::end(*pContainer)));
	}

	template <typename Container>
	LinqEnumerable<detail_::StorageIterator<Container>> from_container(Container container)
	{
		auto pContainer = std::make_shared<Container>(std::move(container));
		return from_pointertocontainer(pContainer);
	}

	template <typename T, size_t size>
	auto from_values(T (&array)[size])
	{
		return from_values(std::vector<T>(array, array + size));
	}

	template <typename T>
	auto from_values(std::initializer_list<T> const& il)
	{
		return from_container(std::vector<T>(il.begin(), il.end()));
	}

	template <typename T>
	LinqEnumerable<detail_::EmptyIterator<T>> from_empty()
	{
		return LinqEnumerable<detail_::EmptyIterator<T>>(detail_::EmptyIterator<T>(), detail_::EmptyIterator<T>());
	}

	template <typename Iter_t, typename Iter2_t, std::enable_if_t<
		std::is_same_v<
			std::decay_t<Iter_t>,
			std::decay_t<Iter_t>
		>, int> = 0>
	auto from(Iter_t begin, Iter2_t end)
	{
		return LinqEnumerable<std::decay_t<Iter_t>>(std::move(begin), std::move(end));
	}

	template <typename Container>
	auto from(Container const& container) -> LinqEnumerable<decltype(std::begin(container))>
	{
		return LinqEnumerable<decltype(std::begin(container))>(std::begin(container), std::end(container));
	}

	template <typename Iter_t>
	auto from(Range<Iter_t> const& range)
	{
		return from(range.begin(), range.end());
	}
}

#ifdef _MSC_VER
#pragma pop_macro("min")
#pragma pop_macro("max")
#endif
