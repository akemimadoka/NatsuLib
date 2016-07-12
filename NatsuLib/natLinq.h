#pragma once
#include "natMisc.h"
#include "natException.h"
#include <memory>
#include <vector>
#include <set>
#include <map>

#pragma push_macro("max")
#ifdef max
#	undef max
#endif
#pragma push_macro("min")
#ifdef min
#	undef min
#endif

template <typename Func>
struct std::hash<std::function<Func>>
{
	size_t operator()(function<Func> const& _Keyval) const
	{
		return hash<decay_t<Func>>()(_Keyval.template target<Func>());
	}
};

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
	namespace _Detail
	{
		template <typename T>
		using deref_t = std::decay_t<decltype(*std::declval<T>())>;

		template <typename T>
		class CommonIterator
		{
			struct IteratorInterface
			{
				virtual ~IteratorInterface() = default;

				virtual void MoveNext() = 0;
				virtual T& Deref() const = 0;
				virtual nBool Equals(IteratorInterface const& other) const = 0;
			};

			template <typename Iter_t>
			class IteratorImpl : public IteratorInterface
			{
				Iter_t m_Iterator;
			public:
				explicit IteratorImpl(Iter_t const& iterator)
					: m_Iterator(iterator)
				{
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

			std::unique_ptr<IteratorInterface> m_Iterator;

		public:
			template <typename Iter_t>
			explicit CommonIterator(Iter_t const& iterator)
				: m_Iterator(std::move(std::make_unique<IteratorImpl<Iter_t>>(iterator)))
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
		class StorageIterator
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
		class EmptyIterator
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
		class SelectIterator
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
		class WhereIterator
		{
			typedef WhereIterator<Iter_t, CallableObj_t> Self_t;

			Iter_t m_Iterator, m_End;
			CallableObj_t m_CallableObj;
		public:
			typedef typename std::iterator_traits<Iter_t>::iterator_category iterator_category;
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

			difference_type operator-(Self_t const& other) const
			{
				return std::distance(other.m_Iterator, m_Iterator);
			}
		};

		template <typename Iter_t>
		class SkipWhileIterator
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
		class TakeIterator
		{
			typedef TakeIterator<Iter_t> Self_t;

			Iter_t m_Iterator, m_Target, m_End;
		public:
			typedef typename std::iterator_traits<Iter_t>::iterator_category iterator_category;
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

				m_Target = std::move(std::next(m_Iterator, count));
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

			difference_type operator-(Self_t const& other) const
			{
				return std::distance(other.m_Iterator, m_Iterator);
			}
		};

		template <typename Iter_t, typename CallableObj_t>
		class TakeWhileIterator
		{
			typedef TakeWhileIterator<Iter_t, CallableObj_t> Self_t;

			Iter_t m_Iterator, m_End;
			CallableObj_t m_CallableObj;
		public:
			typedef typename std::iterator_traits<Iter_t>::iterator_category iterator_category;
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

			difference_type operator-(Self_t const& other) const
			{
				return std::distance(other.m_Iterator, m_Iterator);
			}
		};

		template <typename Iter1_t, typename Iter2_t>
		class ConcatIterator
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
		class ZipIterator
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

	template <typename Iter_t>
	class LinqEnumerable;

	template <typename Container>
	Linq<decltype(*std::begin(std::declval<Container>()))> from_values(std::shared_ptr<Container> const& pContainer);

	template <typename Container>
	Linq<decltype(*std::begin(std::declval<Container>()))> from_values(Container const& container);

	template <typename Container>
	Linq<decltype(*std::begin(std::declval<Container>()))> from_values(Container && container);

	template <typename T, size_t size>
	Linq<T> from_values(T(&array)[size]);

	template <typename T>
	Linq<T> from_values(std::initializer_list<T> const& il);

	template <typename T>
	Linq<T> from_empty();

	template <typename Arg, typename... Args>
	std::enable_if_t<std::disjunction<std::is_same<Arg, Args>...>::value, Linq<Arg>> from_values(Arg&& value, Args&&... values);

	template <typename Iter_t>
	LinqEnumerable<Iter_t> from(Iter_t const& begin, Iter_t const& end);

	template <typename Container>
	auto from(Container const& container)->LinqEnumerable<decltype(*std::begin(container))>;

	template <typename Iter_t>
	class LinqEnumerable
		: public natRange<Iter_t>
	{
		typedef natRange<Iter_t> _Base;
		typedef std::decay_t<decltype(*std::declval<Iter_t>())> Element_t;

	public:
		using _Base::iterator_category;
		using _Base::value_type;
		using _Base::difference_type;
		using _Base::reference;
		using _Base::pointer;

		LinqEnumerable(Iter_t begin, Iter_t end)
			: _Base(begin, end)
		{
		}

		template <typename CallableObj>
		LinqEnumerable<_Detail::SelectIterator<Iter_t, CallableObj>> select(CallableObj const& callableObj) const
		{
			return LinqEnumerable<_Detail::SelectIterator<Iter_t, CallableObj>>(_Detail::SelectIterator<Iter_t, CallableObj>(_Base::begin(), callableObj),
				_Detail::SelectIterator<Iter_t, CallableObj>(_Base::end(), callableObj));
		}

		template <typename CallableObj>
		LinqEnumerable<_Detail::WhereIterator<Iter_t, CallableObj>> where(CallableObj const& callableObj) const
		{
			return LinqEnumerable<_Detail::WhereIterator<Iter_t, CallableObj>>(_Detail::WhereIterator<Iter_t, CallableObj>(_Base::begin(), _Base::end(), callableObj),
				_Detail::WhereIterator<Iter_t, CallableObj>(_Base::end(), _Base::end(), callableObj));
		}

		LinqEnumerable& skip(difference_type count)
		{
			_Base::pop_front(count);
			return *this;
		}

		template <typename CallableObj>
		LinqEnumerable<_Detail::SkipWhileIterator<Iter_t>> skip_while(CallableObj const& callableObj) const
		{
			return LinqEnumerable<_Detail::SkipWhileIterator<Iter_t>>(_Detail::SkipWhileIterator<Iter_t>(_Base::begin(), _Base::end(), callableObj),
				_Detail::SkipWhileIterator<Iter_t>(_Base::end(), _Base::end(), callableObj));
		}

		LinqEnumerable<_Detail::TakeIterator<Iter_t>> take(difference_type count)
		{
			return LinqEnumerable<_Detail::TakeIterator<Iter_t>>(_Detail::TakeIterator<Iter_t>(_Base::begin(), _Base::end(), count),
				_Detail::TakeIterator<Iter_t>(_Base::end(), _Base::end(), count));
		}

		template <typename CallableObj>
		LinqEnumerable<_Detail::TakeWhileIterator<Iter_t, CallableObj>> take_while(CallableObj const& callableObj) const
		{
			return LinqEnumerable<_Detail::TakeWhileIterator<Iter_t, CallableObj>>(_Detail::TakeWhileIterator<Iter_t, CallableObj>(_Base::begin(), _Base::end(), callableObj),
				_Detail::TakeWhileIterator<Iter_t, CallableObj>(_Base::end(), _Base::end(), callableObj));
		}

		template <typename Iter2_t>
		LinqEnumerable<_Detail::ConcatIterator<Iter_t, Iter2_t>> concat(LinqEnumerable<Iter2_t> const& other) const
		{
			return LinqEnumerable<_Detail::ConcatIterator<Iter_t, Iter2_t>>(_Detail::ConcatIterator<Iter_t, Iter2_t>(_Base::begin(), _Base::end(), other.begin(), other.end()),
				_Detail::ConcatIterator<Iter_t, Iter2_t>(_Base::end(), _Base::end(), other.end(), other.end()));
		}

		template <typename T>
		nBool Contains(T const& item) const
		{
			return std::find(_Base::begin(), _Base::end(), item);
		}

		Element_t& element_at(difference_type index)
		{
			return *std::next(_Base::begin(), index);
		}

		Element_t const& element_at(difference_type index) const
		{
			return *std::next(_Base::begin(), index);
		}

		Linq<Element_t> distinct() const
		{
			std::set<Element_t> tmpSet(_Base::begin(), _Base::end());
			return from_values(std::move(tmpSet));
		}

		template <typename Iter2_t>
		Linq<Element_t> except(LinqEnumerable<Iter2_t> const& other) const
		{
			std::set<Element_t> tmpSet(_Base::begin(), _Base::end());
			for (auto&& item : other)
			{
				tmpSet.erase(item);
			}
			return from_values(std::move(tmpSet));
		}

		template <typename Iter2_t>
		Linq<Element_t> intersect(LinqEnumerable<Iter2_t> const& other) const
		{
			std::set<Element_t> tmpSet(_Base::begin(), _Base::end()), tmpSet2(other.begin(), other.end()), result;
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
			if (_Base::empty())
			{
				nat_Throw(natException, _T("Range is empty and there is no default constructor for type Element_t({0})."), natUtil::C2Wstr(typeid(Element_t).name()));
			}

			auto iter = _Base::begin();
			Element_t result = *iter;
			while (iter != _Base::end())
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
			return select(callableObj).aggregate(true, [](nBool a, nBool b) { return a&&b; });
		}

		template <typename CallableObj>
		nBool any(CallableObj const& callableObj) const
		{
			return !where(callableObj).empty();
		}

		template <typename Result_t = Element_t>
		Result_t average() const
		{
			Result_t result{};
			return aggregate(result, [](Result_t const& res, Element_t const& item)
			{
				return res + item;
			}) / static_cast<Result_t>(_Base::size());
		}

		Element_t max() const
		{
			return aggregate([](const Element_t& a, const Element_t& b) { return a > b ? a : b; });
		}

		Element_t min() const
		{
			return aggregate([](const Element_t& a, const Element_t& b) { return a < b ? a : b; });
		}

		Element_t sum() const
		{
			return aggregate(0, [](const Element_t& a, const Element_t& b) { return a + b; });
		}

		Element_t product() const
		{
			return aggregate([](const Element_t& a, const Element_t& b) { return a * b; });
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
			return LinqEnumerable<_Detail::ZipIterator<Iter_t, Iter2_t>>(_Detail::ZipIterator<Iter_t, Iter2_t>(_Base::begin(), _Base::end(), e.begin(), e.end()),
				_Detail::ZipIterator<Iter_t, Iter2_t>(_Base::end(), _Base::end(), e.end(), e.end()));
		}

		template <typename Container>
		Container Cast() const
		{
			return std::move(Container(_Base::begin(), _Base::end()));
		}
	};

	template <typename T>
	class Linq
		: public LinqEnumerable<_Detail::CommonIterator<T>>
	{
		typedef LinqEnumerable<_Detail::CommonIterator<T>> _Base;
	public:
		template <typename Iter_t>
		Linq(LinqEnumerable<Iter_t> const& linqEnumerable)
			: _Base(_Detail::CommonIterator<T>(linqEnumerable.begin()),
				_Detail::CommonIterator<T>(linqEnumerable.end()))
		{
		}
	};

	template <typename Container>
	Linq<decltype(*std::begin(std::declval<Container>()))> from_values(std::shared_ptr<Container> const& pContainer)
	{
		return LinqEnumerable<_Detail::StorageIterator<Container>>(_Detail::StorageIterator<Container>(pContainer, std::begin(*pContainer)),
			_Detail::StorageIterator<Container>(pContainer, std::end(*pContainer)));
	}

	template <typename Container>
	Linq<decltype(*std::begin(std::declval<Container>()))> from_values(Container const& container)
	{
		auto pContainer = std::make_shared<Container>(container);
		return from_values(pContainer);
	}

	template <typename Container>
	Linq<decltype(*std::begin(std::declval<Container>()))> from_values(Container && container)
	{
		auto pContainer = std::make_shared<Container>(std::move(container));
		return from_values(pContainer);
	}

	template <typename T, size_t size>
	Linq<T> from_values(T (&array)[size])
	{
		return from_values(std::move(std::vector<T>(array, array + size)));
	}

	template <typename T>
	Linq<T> from_values(std::initializer_list<T> const& il)
	{
		return from_values(std::move(std::vector<T>(il.begin(), il.end())));
	}

	template <typename T>
	Linq<T> from_empty()
	{
		return LinqEnumerable<_Detail::EmptyIterator<T>>(_Detail::EmptyIterator<T>(), _Detail::EmptyIterator<T>());
	}

	template <typename Arg, typename... Args>
	std::enable_if_t<std::disjunction<std::is_same<Arg, Args>...>::value, Linq<Arg>> from_values(Arg&& value, Args&&... values)
	{
		return from_values({ value, values... });
	}

	template <typename Iter_t>
	LinqEnumerable<Iter_t> from(Iter_t const& begin, Iter_t const& end)
	{
		return LinqEnumerable<Iter_t>(begin, end);
	}

	template <typename Container>
	auto from(Container const& container) -> LinqEnumerable<decltype(*std::begin(container))>
	{
		return LinqEnumerable<decltype(*std::begin(container))>(std::begin(container), std::end(container));
	}

	template <typename Iter_t>
	LinqEnumerable<Iter_t> from(natRange<Iter_t> const& range)
	{
		return from(range.begin(), range.end());
	}
}

#pragma pop_macro("min")
#pragma pop_macro("max")
