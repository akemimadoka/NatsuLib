#pragma once
#include "natMisc.h"
#include "natException.h"
#include <memory>
#include <vector>
#include <set>
#include <map>

#ifdef _MSC_VER
#	pragma push_macro("max")
#	pragma push_macro("min")
#	undef max
#	undef min
#endif

namespace std
{
	template <typename Func>
	struct hash<function<Func>>
	{
		size_t operator()(function<Func> const& _Keyval) const
		{
			return hash<decay_t<Func>>()(_Keyval.template target<Func>());
		}
	};
}

template <typename Func>
bool operator==(std::function<Func> const& left, std::function<Func> const& right)
{
	return left.template target<Func>() == right.template target<Func>();
}

template <typename Func>
bool operator!=(std::function<Func> const& left, std::function<Func> const& right)
{
	return !(left == right);
}

template <typename Func1, typename Func2>
bool operator==(std::function<Func1> const&, std::function<Func2> const&)
{
	return false;
}

template <typename Func1, typename Func2>
bool operator!=(std::function<Func1> const&, std::function<Func2> const&)
{
	return true;
}

namespace NatsuLib
{
	namespace detail_
	{
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

		template <typename T, typename Result_t, bool Test = std::conjunction<Addable<Result_t, typename T::Element_t>, Dividable<Result_t>>::value>
		struct GetAverage
		{
			[[noreturn]] static void Get(T const& self)
			{
				nat_Throw(natException, _T("Cannot apply such operation to this type."));
			}
		};

		template <typename T, typename Result_t>
		struct GetAverage<T, Result_t, true>
		{
			static Result_t Get(T const& self)
			{
				Result_t result{};
				return self.aggregate(result, [](Result_t const& res, typename T::Element_t const& item)
				{
					return res + item;
				}) / static_cast<Result_t>(self.m_Range.size());
			}
		};

		template <typename T, bool Test = CanGreater<typename T::Element_t>::value>
		struct GetMax
		{
			[[noreturn]] static void Get(T const& self)
			{
				nat_Throw(natException, _T("Cannot apply such operation to this type."));
			}
		};

		template <typename T>
		struct GetMax<T, true>
		{
			static typename T::Element_t Get(T const& self)
			{
				return self.aggregate([](const typename T::Element_t& a, const typename T::Element_t& b) { return a > b ? a : b; });
			}
		};

		template <typename T, bool Test = CanLesser<typename T::Element_t>::value>
		struct GetMin
		{
			[[noreturn]] static void Get(T const& self)
			{
				nat_Throw(natException, _T("Cannot apply such operation to this type."));
			}
		};

		template <typename T>
		struct GetMin<T, true>
		{
			static typename T::Element_t Get(T const& self)
			{
				return self.aggregate([](const typename T::Element_t& a, const typename T::Element_t& b) { return a < b ? a : b; });
			}
		};

		template <typename T, bool Test = Addable<typename T::Element_t>::value>
		struct GetSum
		{
			[[noreturn]] static void Get(T const& self)
			{
				nat_Throw(natException, _T("Cannot apply such operation to this type."));
			}
		};

		template <typename T>
		struct GetSum<T, true>
		{
			static typename T::Element_t Get(T const& self)
			{
				return self.aggregate(0, [](const typename T::Element_t& a, const typename T::Element_t& b) { return a + b; });
			}
		};

		template <typename T, bool Test = Multipliable<typename T::Element_t>::value>
		struct GetProduct
		{
			[[noreturn]] static void Get(T const& self)
			{
				nat_Throw(natException, _T("Cannot apply such operation to this type."));
			}
		};

		template <typename T>
		struct GetProduct<T, true>
		{
			static typename T::Element_t Get(T const& self)
			{
				return self.aggregate(0, [](const typename T::Element_t& a, const typename T::Element_t& b) { return a * b; });
			}
		};

		template <typename T>
		using deref_t = std::decay_t<decltype(*std::declval<T>())>;

		template <typename T>
		class CommonIterator final
		{
			struct IteratorInterface
			{
				virtual ~IteratorInterface() = default;

				virtual std::shared_ptr<IteratorInterface> Clone() const = 0;
				virtual void MoveNext() = 0;
				virtual T& Deref() const = 0;
				virtual nBool Equals(IteratorInterface const& other) const = 0;
			};

			template <typename Iter_t>
			class IteratorImpl final : public IteratorInterface
			{
				Iter_t m_Iterator;
			public:
				explicit IteratorImpl(Iter_t const& iterator)
					: m_Iterator(iterator)
				{
				}

				std::shared_ptr<IteratorInterface> Clone() const override
				{
					return std::static_pointer_cast<IteratorInterface>(std::make_shared<IteratorImpl>(m_Iterator));
				}

				void MoveNext() override
				{
					++m_Iterator;
				}

				T& Deref() const override
				{
					return *m_Iterator;
				}

				nBool Equals(IteratorInterface const& other) const override
				{
					return m_Iterator == dynamic_cast<IteratorImpl const&>(other).m_Iterator;
				}
			};

			typedef CommonIterator<T> Self_t;

			std::shared_ptr<IteratorInterface> m_Iterator;

		public:
			typedef std::forward_iterator_tag iterator_category;
			typedef std::remove_reference_t<T> value_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::add_lvalue_reference_t<value_type> reference;
			typedef std::add_pointer_t<value_type> pointer;

			template <typename Iter_t>
			explicit CommonIterator(Iter_t const& iterator)
				: m_Iterator(std::make_shared<IteratorImpl<Iter_t>>(iterator))
			{
			}

			CommonIterator(Self_t const& other)
				: m_Iterator(other.m_Iterator->Clone())
			{
			}

			Self_t& operator++() &
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

			difference_type operator-(Self_t const& other) const
			{
				return std::distance(other.m_Iterator, m_Iterator);
			}
		};

		template <typename T>
		class EmptyIterator final
		{
			typedef EmptyIterator<T> Self_t;
		public:
			typedef void iterator_category;
			typedef void value_type;
			typedef void difference_type;
			typedef void reference;
			typedef void pointer;

			Self_t& operator++() &
			{
				return *this;
			}

			[[noreturn]] T& operator*() const
			{
				nat_Throw(natException, _T("Try to deref an empty iterator."));
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

		template <typename Iter_t, typename CallableObj_t>
		class SelectIterator final
		{
			typedef SelectIterator<Iter_t, CallableObj_t> Self_t;

			Iter_t m_Iterator;
			CallableObj_t m_CallableObj;

		public:
			typedef typename std::iterator_traits<Iter_t>::iterator_category iterator_category;
			typedef typename std::iterator_traits<Iter_t>::value_type value_type;
			typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
			typedef typename std::iterator_traits<Iter_t>::reference reference;
			typedef typename std::iterator_traits<Iter_t>::pointer pointer;

			SelectIterator(Iter_t const& iterator, CallableObj_t const& callableObj)
				: m_Iterator(iterator), m_CallableObj(callableObj)
			{
			}

			Self_t& operator++() &
			{
				++m_Iterator;
				return *this;
			}

			decltype(auto) operator*() const
			{
				return m_CallableObj(*m_Iterator);
			}

			nBool operator==(Self_t const& other) const
			{
				return m_Iterator == other.m_Iterator/* && m_CallableObj == other.m_CallableObj*/;
			}

			nBool operator!=(Self_t const& other) const
			{
				return !(*this == other);
			}

			difference_type operator-(Self_t const& other) const
			{
				return std::distance(other.m_Iterator, m_Iterator);
			}
		};

		template <typename Iter_t, typename CallableObj_t>
		class WhereIterator final
		{
			typedef WhereIterator<Iter_t, CallableObj_t> Self_t;

			Iter_t m_Iterator, m_End;
			CallableObj_t m_CallableObj;
		public:
			typedef std::forward_iterator_tag iterator_category;
			typedef typename std::iterator_traits<Iter_t>::value_type value_type;
			typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
			typedef typename std::iterator_traits<Iter_t>::reference reference;
			typedef typename std::iterator_traits<Iter_t>::pointer pointer;

			WhereIterator(Iter_t const& iterator, Iter_t const& end, CallableObj_t const& callableObj)
				: m_Iterator(iterator), m_End(end), m_CallableObj(callableObj)
			{
				while (m_Iterator != m_End && !m_CallableObj(*m_Iterator))
				{
					++m_Iterator;
				}
			}

			Self_t& operator++() &
			{
				++m_Iterator;
				while (m_Iterator != m_End && !m_CallableObj(*m_Iterator))
				{
					++m_Iterator;
				}

				return *this;
			}

			decltype(auto) operator*() const
			{
				return *m_Iterator;
			}

			nBool operator==(Self_t const& other) const
			{
				return m_Iterator == other.m_Iterator && m_End == other.m_End/* && m_CallableObj == other.m_CallableObj*/;
			}

			nBool operator!=(Self_t const& other) const
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
			SkipWhileIterator(Iter_t const& iterator, Iter_t const& end, CallableObj_t const& callableObj)
				: m_Iterator(iterator)
			{
				while (m_Iterator != end && callableObj(*m_Iterator))
				{
					++m_Iterator;
				}
			}

			Self_t& operator++() &
			{
				++m_Iterator;
				return *this;
			}

			decltype(auto) operator*() const
			{
				return *m_Iterator;
			}

			nBool operator==(Self_t const& other) const
			{
				return m_Iterator == other.m_Iterator;
			}

			nBool operator!=(Self_t const& other) const
			{
				return !(*this == other);
			}

			difference_type operator-(Self_t const& other) const
			{
				return std::distance(other.m_Iterator, m_Iterator);
			}
		};

		template <typename Iter_t>
		class TakeIterator final
		{
			typedef TakeIterator<Iter_t> Self_t;

			Iter_t m_Iterator, m_Target, m_End;
		public:
			typedef std::forward_iterator_tag iterator_category;
			typedef typename std::iterator_traits<Iter_t>::value_type value_type;
			typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
			typedef typename std::iterator_traits<Iter_t>::reference reference;
			typedef typename std::iterator_traits<Iter_t>::pointer pointer;

			TakeIterator(Iter_t const& iterator, Iter_t const& end, difference_type count)
				: m_Iterator(iterator), m_End(end)
			{
				if (std::distance(m_Iterator, m_End) < count)
				{
					nat_Throw(natException, _T("Out of range."));
				}

				m_Target = std::next(m_Iterator, count);
			}

			Self_t& operator++() &
			{
				if (++m_Iterator == m_Target)
				{
					m_Iterator = m_End;
				}
				
				return *this;
			}

			decltype(auto) operator*() const
			{
				return *m_Iterator;
			}

			nBool operator==(Self_t const& other) const
			{
				return m_Iterator == other.m_Iterator && m_Target == other.m_Target && m_End == other.m_End;
			}

			nBool operator!=(Self_t const& other) const
			{
				return !(*this == other);
			}
		};

		template <typename Iter_t, typename CallableObj_t>
		class TakeWhileIterator final
		{
			typedef TakeWhileIterator<Iter_t, CallableObj_t> Self_t;

			Iter_t m_Iterator, m_End;
			CallableObj_t m_CallableObj;
		public:
			typedef std::forward_iterator_tag iterator_category;
			typedef typename std::iterator_traits<Iter_t>::value_type value_type;
			typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
			typedef typename std::iterator_traits<Iter_t>::reference reference;
			typedef typename std::iterator_traits<Iter_t>::pointer pointer;

			TakeWhileIterator(Iter_t const& iterator, Iter_t const& end, CallableObj_t const& callableObj)
				: m_Iterator(iterator), m_End(end), m_CallableObj(callableObj)
			{
				if (m_Iterator != m_End && !m_CallableObj(*m_Iterator))
				{
					m_Iterator = m_End;
				}
			}

			Self_t& operator++() &
			{
				if (!m_CallableObj(*++m_Iterator))
				{
					m_Iterator = m_End;
				}

				return *this;
			}

			decltype(auto) operator*() const
			{
				return *m_Iterator;
			}

			nBool operator==(Self_t const& other) const
			{
				return m_Iterator == other.m_Iterator && m_End == other.m_End/* && m_CallableObj == other.m_CallableObj*/;
			}

			nBool operator!=(Self_t const& other) const
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

			typedef std::forward_iterator_tag iterator_category;
			typedef std::common_type_t<value_type1, value_type2> value_type;
			typedef std::common_type_t<difference_type1, difference_type2> difference_type;
			typedef std::common_type_t<reference1, reference2> reference;
			typedef std::common_type_t<pointer1, pointer2> pointer;

			ConcatIterator(Iter1_t const& current1, Iter1_t const& end1, Iter2_t const& current2, Iter2_t const& end2)
				: m_Current1(current1), m_End1(end1), m_Current2(current2), m_End2(end2)
			{
			}

			Self_t& operator++() &
			{
				++(m_Current1 != m_End1 ? m_Current1 : m_Current2);

				return *this;
			}

			decltype(auto) operator*() const
			{
				return m_Current1 != m_End1 ? *m_Current1 : *m_Current2;
			}

			nBool operator==(Self_t const& other) const
			{
				return m_Current1 == other.m_Current1 && m_End1 == other.m_End1 && m_Current2 == other.m_Current2 && m_End2 == other.m_End2;
			}

			nBool operator!=(Self_t const& other) const
			{
				return !(*this == other);
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

			typedef std::forward_iterator_tag iterator_category;
			typedef std::common_type_t<value_type1, value_type2> value_type;
			typedef std::common_type_t<difference_type1, difference_type2> difference_type;
			typedef std::common_type_t<reference1, reference2> reference;
			typedef std::common_type_t<pointer1, pointer2> pointer;

		private:
			typedef std::pair<value_type1, value_type2> Element_t;

		public:
			ZipIterator(Iter1_t const& current1, Iter1_t const& end1, Iter2_t const& current2, Iter2_t const& end2)
				: m_Current1(current1), m_End1(end1), m_Current2(current2), m_End2(end2)
			{
			}

			Self_t operator++() &
			{
				if (m_Current1 != m_End1 && m_Current2 != m_End2)
				{
					++m_Current1;
					++m_Current2;
				}

				return *this;
			}

			Element_t operator*() const
			{
				return Element_t(*m_Current1, *m_Current2);
			}

			nBool operator==(Self_t const& other) const
			{
				return m_Current1 == other.m_Current1 && m_End1 == other.m_End1 && m_Current2 == other.m_Current2 && m_End2 == other.m_End2;
			}

			nBool operator!=(Self_t const& other) const
			{
				return !(*this == other);
			}
		};
	}

	template <typename T>
	class Linq;

	template <typename Container>
	Linq<decltype(*std::begin(std::declval<Container>()))> from_values(Container && container);

	template <typename T>
	Linq<T> from_empty();

	template <typename Iter_t>
	class LinqEnumerable
	{
		typedef std::decay_t<decltype(*std::declval<Iter_t>())> Element_t;

	public:
		typedef typename std::iterator_traits<Iter_t>::iterator_category iterator_category;
		typedef typename std::iterator_traits<Iter_t>::value_type value_type;
		typedef typename std::iterator_traits<Iter_t>::difference_type difference_type;
		typedef typename std::iterator_traits<Iter_t>::reference reference;
		typedef typename std::iterator_traits<Iter_t>::pointer pointer;

		typedef LinqEnumerable<Iter_t> Self_t;

	private:
		Range<Iter_t> m_Range;
		mutable Optional<size_t> m_Size;

	public:
		constexpr LinqEnumerable(Iter_t begin, Iter_t end)
			: m_Range(begin, end)
		{
		}

		constexpr LinqEnumerable(LinqEnumerable const& other)
			: m_Range(other.begin(), other.end())
		{
		}

		LinqEnumerable& operator=(LinqEnumerable const& other)
		{
			m_Range = other.m_Range;
			m_Size = other.m_Size;

			return *this;
		}

		constexpr Iter_t begin() const
		{
			return m_Range.begin();
		}

		constexpr Iter_t end() const
		{
			return m_Range.end();
		}

		size_t size() const
		{
			if (!m_Size)
			{
				m_Size.emplace(static_cast<size_t>(std::distance(m_Range.begin(), m_Range.end())));
			}

			return m_Size.value();
		}

		size_t count() const
		{
			return size();
		}

		template <typename CallableObj>
		LinqEnumerable<detail_::SelectIterator<Iter_t, CallableObj>> select(CallableObj const& callableObj) const
		{
			return LinqEnumerable<detail_::SelectIterator<Iter_t, CallableObj>>(detail_::SelectIterator<Iter_t, CallableObj>(m_Range.begin(), callableObj),
				detail_::SelectIterator<Iter_t, CallableObj>(m_Range.end(), callableObj));
		}

		template <typename CallableObj>
		LinqEnumerable<detail_::WhereIterator<Iter_t, CallableObj>> where(CallableObj const& callableObj) const
		{
			return LinqEnumerable<detail_::WhereIterator<Iter_t, CallableObj>>(detail_::WhereIterator<Iter_t, CallableObj>(m_Range.begin(), m_Range.end(), callableObj),
				detail_::WhereIterator<Iter_t, CallableObj>(m_Range.end(), m_Range.end(), callableObj));
		}

		LinqEnumerable skip(difference_type count)
		{
			LinqEnumerable ret(*this);
			ret.m_Range.pop_front(count);
			return ret;
		}

		template <typename CallableObj>
		LinqEnumerable<detail_::SkipWhileIterator<Iter_t>> skip_while(CallableObj const& callableObj) const
		{
			return LinqEnumerable<detail_::SkipWhileIterator<Iter_t>>(detail_::SkipWhileIterator<Iter_t>(m_Range.begin(), m_Range.end(), callableObj),
				detail_::SkipWhileIterator<Iter_t>(m_Range.end(), m_Range.end(), callableObj));
		}

		LinqEnumerable<detail_::TakeIterator<Iter_t>> take(difference_type count)
		{
			return LinqEnumerable<detail_::TakeIterator<Iter_t>>(detail_::TakeIterator<Iter_t>(m_Range.begin(), m_Range.end(), count),
				detail_::TakeIterator<Iter_t>(m_Range.end(), m_Range.end(), count));
		}

		template <typename CallableObj>
		LinqEnumerable<detail_::TakeWhileIterator<Iter_t, CallableObj>> take_while(CallableObj const& callableObj) const
		{
			return LinqEnumerable<detail_::TakeWhileIterator<Iter_t, CallableObj>>(detail_::TakeWhileIterator<Iter_t, CallableObj>(m_Range.begin(), m_Range.end(), callableObj),
				detail_::TakeWhileIterator<Iter_t, CallableObj>(m_Range.end(), m_Range.end(), callableObj));
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

		Element_t& element_at(difference_type index)
		{
			return *std::next(m_Range.begin(), index);
		}

		Element_t const& element_at(difference_type index) const
		{
			return *std::next(m_Range.begin(), index);
		}

		Linq<Element_t> distinct() const
		{
			std::set<Element_t> tmpSet(m_Range.begin(), m_Range.end());
			return from_values(std::move(tmpSet));
		}

		template <typename Iter2_t>
		Linq<Element_t> except(LinqEnumerable<Iter2_t> const& other) const
		{
			std::set<Element_t> tmpSet(m_Range.begin(), m_Range.end());
			for (auto&& item : other)
			{
				tmpSet.erase(item);
			}
			return from_values(std::move(tmpSet));
		}

		template <typename Iter2_t>
		Linq<Element_t> intersect(LinqEnumerable<Iter2_t> const& other) const
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
			return from_values(std::move(result));
		}

		template <typename Iter2_t>
		Linq<Element_t> union_with(LinqEnumerable<Iter2_t> const& other) const
		{
			return concat(other).distinct();
		}

		template <typename CallableObj>
		Element_t aggregate(std::enable_if_t<!std::is_default_constructible<Element_t>::value, CallableObj const&> callableObj) const
		{
			if (m_Range.empty())
			{
				nat_Throw(natException, _T("Range is empty and there is no default constructor for this type."));
			}

			auto iter = m_Range.begin();
			Element_t result = *iter;
			while (iter != m_Range.end())
			{
				result = callableObj(result, *iter);
			}
			return result;
		}

		template <typename CallableObj>
		Element_t aggregate(std::enable_if_t<std::is_default_constructible<Element_t>::value, CallableObj const&> callableObj) const
		{
			return aggregate(Element_t{}, callableObj);
		}

		template <typename Result_t, typename CallableObj>
		Result_t aggregate(Result_t result, CallableObj const& callableObj) const
		{
			for (auto&& item : *this)
			{
				result = callableObj(result, item);
			}
			return result;
		}

		template <typename CallableObj>
		nBool all(CallableObj const& callableObj) const
		{
			return select(callableObj).aggregate(true, [](nBool a, nBool b) { return a && b; });
		}

		template <typename CallableObj>
		nBool any(CallableObj const& callableObj) const
		{
			return !where(callableObj).empty();
		}

		template <typename Result_t = Element_t>
		auto average() const
		{
			return detail_::GetAverage<Self_t, Result_t>::Get(*this);
		}

		auto max() const
		{
			return detail_::GetMax<Self_t>::Get(*this);
		}

		auto min() const
		{
			return detail_::GetMin<Self_t>::Get(*this);
		}

		auto sum() const
		{
			return detail_::GetSum<Self_t>::Get(*this);
		}

		auto product() const
		{
			return detail_::GetProduct<Self_t>::Get(*this);
		}

		template <typename CallableObj>
		auto select_many(CallableObj const& callableObj) const
		{
			typedef decltype(callableObj(std::declval<Element_t>())) Collection_t;
			typedef decltype(*std::begin(std::declval<Collection_t>())) Value_t;
			return select(callableObj).aggregate(from_empty<Value_t>(), [](Linq<Value_t> const& a, Collection_t const& b)
			{
				return a.concat(b);
			});
		}

		template <typename CallableObj>
		auto group_by(CallableObj const& keySelector) const
		{
			typedef decltype(keySelector(std::declval<Element_t>())) Key_t;
			std::map<Key_t, std::vector<Element_t>> tmpMap;
			for (auto&& item : *this)
			{
				auto&& key = keySelector(item);
				tmpMap[key].emplace_back(item);
			}

			std::vector<std::pair<Key_t, Linq<Element_t>>> result;
			for (auto&& item : tmpMap)
			{
				result.emplace_back(std::make_pair<Key_t, Linq<Element_t>>(item.first, from_values(std::move(item.second))));
			}

			return from_values(std::move(result));
		}

		template <typename Iter2_t, typename CallableObj1, typename CallableObj2>
		auto full_join(LinqEnumerable<Iter2_t> const& e, CallableObj1 const& keySelector1, CallableObj2 const& keySelector2) const
		{
			typedef std::remove_reference_t<decltype(keySelector1(std::declval<Element_t>()))> Key1_t;
			typedef std::remove_reference_t<decltype(keySelector2(std::declval<Element_t>()))> Key2_t;
			static_assert(std::is_same<Key1_t, Key2_t>::value, "Key1_t and Key2_t should be same.");
			typedef std::common_type_t<Key1_t, Key2_t> Key_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter_t>())>> Value1_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter2_t>())>> Value2_t;
			typedef std::tuple<Key_t, Linq<Element_t>, Linq<Element_t>> FullJoinResult_t;

			std::multimap<Key_t, Value1_t> map1;
			std::multimap<Key_t, Value2_t> map2;

			for (auto&& item : *this)
			{
				map1.insert(std::make_pair(keySelector1(item), item));
			}

			for (auto&& item : e)
			{
				map2.insert(std::make_pair(keySelector2(item), item));
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
					result.emplace_back(std::make_tuple(key1, from_values(std::move(outers)), from_empty<Value2_t>()));
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
					result.emplace_back(std::make_tuple(key2, from_empty<Value1_t>(), from_values(std::move(inners))));
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
					result.emplace_back(std::make_tuple(key1, from_values(std::move(outers)), from_values(std::move(inners))));
					lower1 = upper1;
					lower2 = upper2;
				}
			}

			return from_values(std::move(result));
		}

		template <typename Iter2_t, typename CallableObj1, typename CallableObj2>
		auto group_join(LinqEnumerable<Iter2_t> const& e, CallableObj1 const& keySelector1, CallableObj2 const& keySelector2) const
		{
			typedef std::remove_reference_t<decltype(keySelector1(std::declval<Element_t>()))> Key1_t;
			typedef std::remove_reference_t<decltype(keySelector2(std::declval<Element_t>()))> Key2_t;
			static_assert(std::is_same<Key1_t, Key2_t>::value, "Key1_t and Key2_t should be same.");
			typedef std::common_type_t<Key1_t, Key2_t> Key_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter_t>())>> Value1_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter2_t>())>> Value2_t;
			typedef std::tuple<Key_t, Linq<Element_t>, Linq<Element_t>> FullJoinResult_t;
			typedef std::tuple<Key_t, Value1_t, Linq<Value2_t>> GroupJoinResult_t;

			return full_join(e, keySelector1, keySelector2).select_many([](const FullJoinResult_t& item)
			{
				Linq<Value1_t> outers = item.second.first;
				return outers.select([item](const Value1_t& outer)
				{
					return std::make_tuple(item.first, outer, item.second.second);
				});
			});
		}

		template <typename Iter2_t, typename CallableObj1, typename CallableObj2>
		auto join(LinqEnumerable<Iter2_t> const& e, CallableObj1 const& keySelector1, CallableObj2 const& keySelector2) const
		{
			typedef std::remove_reference_t<decltype(keySelector1(std::declval<Element_t>()))> Key1_t;
			typedef std::remove_reference_t<decltype(keySelector2(std::declval<Element_t>()))> Key2_t;
			static_assert(std::is_same<Key1_t, Key2_t>::value, "Key1_t and Key2_t should be same.");
			typedef std::common_type_t<Key1_t, Key2_t> Key_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter_t>())>> Value1_t;
			typedef std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iter2_t>())>> Value2_t;
			typedef std::tuple<Key_t, Value1_t, Linq<Value2_t>> GroupJoinResult_t;
			typedef std::tuple<Key_t, Value1_t, Value2_t> JoinResult_t;

			return group_join(e, keySelector1, keySelector2).select_many([](const GroupJoinResult_t& item)
			{
				Linq<Value2_t> inners = item.second.second;
				return inners.select([item](const Value2_t& inner)
				{
					return std::make_tuple(item.first, item.second.first, inner);
				});
			});
		}

		template <typename CallableObj>
		auto first_order_by(CallableObj const& keySelector) const
		{
			typedef std::remove_reference_t<decltype(keySelector(std::declval<Element_t>()))> Key_t;

			return group_by(keySelector).select([](std::pair<Key_t, Linq<Element_t>> const& p)
			{
				return p.second;
			});
		}

		template <typename CallableObj>
		auto then_order_by(CallableObj const& keySelector) const
		{
			return select_many([keySelector](Element_t const& values)
			{
				return values.first_order_by(keySelector);
			});
		}

		template <typename CallableObj>
		auto order_by(CallableObj const& keySelector) const
		{
			return first_order_by(keySelector).select_many([](const Linq<Element_t>& values) { return values; });
		}

		template <typename Iter2_t>
		auto zip(LinqEnumerable<Iter2_t> const& e) const
		{
			return LinqEnumerable<detail_::ZipIterator<Iter_t, Iter2_t>>(detail_::ZipIterator<Iter_t, Iter2_t>(m_Range.begin(), m_Range.end(), e.begin(), e.end()),
				detail_::ZipIterator<Iter_t, Iter2_t>(m_Range.end(), m_Range.end(), e.end(), e.end()));
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
		typedef LinqEnumerable<detail_::CommonIterator<T>> _Base;
	public:
		template <typename Iter_t>
		Linq(LinqEnumerable<Iter_t> const& linqEnumerable)
			: _Base(detail_::CommonIterator<T>(linqEnumerable.begin()),
				detail_::CommonIterator<T>(linqEnumerable.end()))
		{
		}
	};

	template <typename Container>
	auto from_pointertocontainer(std::shared_ptr<Container> const& pContainer)
	{
		return LinqEnumerable<detail_::StorageIterator<Container>>(detail_::StorageIterator<Container>(pContainer, std::begin(*pContainer)),
			detail_::StorageIterator<Container>(pContainer, std::end(*pContainer)));
	}

	template <typename Container>
	auto from_values(Container const& container)
	{
		auto pContainer = std::make_shared<Container>(container);
		return from_pointertocontainer(pContainer);
	}

	template <typename Container>
	Linq<decltype(*std::begin(std::declval<Container>()))> from_values(Container && container)
	{
		auto pContainer = std::make_shared<Container>(std::move(container));
		return from_pointertocontainer(pContainer);
	}

	template <typename T, size_t size>
	Linq<T> from_values(T (&array)[size])
	{
		return from_values(std::vector<T>(array, array + size));
	}

	template <typename T>
	Linq<T> from_values(std::initializer_list<T> const& il)
	{
		return from_values(std::vector<T>(il.begin(), il.end()));
	}

	template <typename T>
	Linq<T> from_empty()
	{
		return LinqEnumerable<detail_::EmptyIterator<T>>(detail_::EmptyIterator<T>(), detail_::EmptyIterator<T>());
	}

	template <typename Arg, typename... Args, std::enable_if_t<std::conjunction<std::is_same<Arg, Args>...>::value, nBool> = true>
	auto from_values(Arg&& value, Args&&... values)
	{
		std::vector<std::remove_reference_t<std::remove_cv_t<Arg>>> tmpVec { std::forward<Arg>(value), std::forward<Args>(values)... };
		return from_values(std::move(tmpVec));
	}

	template <typename Iter_t>
	LinqEnumerable<Iter_t> from(Iter_t const& begin, Iter_t const& end)
	{
		return LinqEnumerable<Iter_t>(begin, end);
	}

	template <typename Container>
	auto from(Container const& container) -> LinqEnumerable<decltype(std::begin(container))>
	{
		return LinqEnumerable<decltype(std::begin(container))>(std::begin(container), std::end(container));
	}

	template <typename Iter_t>
	LinqEnumerable<Iter_t> from(Range<Iter_t> const& range)
	{
		return from(range.begin(), range.end());
	}
}

#ifdef _MSC_VER
#pragma pop_macro("min")
#pragma pop_macro("max")
#endif

