#pragma once
#include "natConfig.h"
#include "natRefObj.h"
#include "natMisc.h"
#include "natLinq.h"

#ifdef _MSC_VER
#pragma push_macro("max")
#endif
#undef max

namespace NatsuLib
{
	namespace detail_
	{
		struct DummyType
		{
			template <typename T>
			[[noreturn]] nBool operator==(T const&) const
			{
				nat_Throw(natErrException, NatErr_NotSupport, "DummyType"_nv);
			}

			template <typename T>
			[[noreturn]] nBool operator!=(T const&) const
			{
				nat_Throw(natErrException, NatErr_NotSupport, "DummyType"_nv);
			}

			[[noreturn]] DummyType operator++() const
			{
				nat_Throw(natErrException, NatErr_NotSupport, "DummyType"_nv);
			}

			[[noreturn]] DummyType operator++(int) const
			{
				nat_Throw(natErrException, NatErr_NotSupport, "DummyType"_nv);
			}

			[[noreturn]] DummyType operator--() const
			{
				nat_Throw(natErrException, NatErr_NotSupport, "DummyType"_nv);
			}

			[[noreturn]] DummyType operator--(int) const
			{
				nat_Throw(natErrException, NatErr_NotSupport, "DummyType"_nv);
			}

			[[noreturn]] DummyType operator*() const
			{
				nat_Throw(natErrException, NatErr_NotSupport, "DummyType"_nv);
			}

			template <typename T>
			operator T&() const
			{
				nat_Throw(natErrException, NatErr_NotSupport, "DummyType"_nv);
			}

			template <typename T>
			operator T&&() const
			{
				nat_Throw(natErrException, NatErr_NotSupport, "DummyType"_nv);
			}
		};

		template <typename C, nBool = std::is_const<C>::value>
		struct ConstContainerConcept
		{
			static constexpr nBool IsConst = false;

			static void Clone(C& a, const C& b)
			{
				a = b;
			}

			static void Move(C& a, C&& b)
			{
				a = std::move(b);
			}

			static void Swap(C& a, C& b)
			{
				using std::swap;
				swap(a, b);
			}
		};

		template <typename C>
		struct ConstContainerConcept<C, true>
		{
			static constexpr nBool IsConst = true;

			[[noreturn]] static void Clone(C&, const C&)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This container is const."_nv);
			}

			[[noreturn]] static void Move(C&, C&&)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This container is const."_nv);
			}

			[[noreturn]] static void Swap(C&, C&)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This container is const."_nv);
			}
		};

		template <typename C, typename = void>
		struct ContainerWithSizeConcept
		{
			typedef DummyType size_type;

			static constexpr nBool HasSize = false;

			[[noreturn]] static size_type size(C const&)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This container does not have size."_nv);
			}
		};

		template <typename C>
		struct ContainerWithSizeConcept<C, std::void_t<typename C::size_type, decltype(std::declval<std::add_const_t<C>>().size())>>
		{
			typedef typename C::size_type size_type;

			static constexpr nBool HasSize = true;

			static size_type size(C const& container)
			{
				return container.size();
			}
		};

		template <typename C, typename = void>
		struct ReversibleContainerConcept
		{
			typedef DummyType reverse_iterator;
			typedef DummyType const_reverse_iterator;

			static constexpr nBool IsReversibleContainer = false;

			[[noreturn]] static reverse_iterator rbegin(C&)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This container is not a ReversibleContainer."_nv);
			}

			[[noreturn]] static reverse_iterator rend(C&)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This container is not a ReversibleContainer."_nv);
			}

			[[noreturn]] static const_reverse_iterator crbegin(C const&)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This container is not a ReversibleContainer."_nv);
			}

			[[noreturn]] static const_reverse_iterator crend(C const&)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This container is not a ReversibleContainer."_nv);
			}
		};

		template <typename C>
		struct ReversibleContainerConcept<C, 
			std::void_t<
				typename C::reverse_iterator,
				typename C::const_reverse_iterator,
				decltype(std::declval<C>().rbegin()),
				decltype(std::declval<std::add_const_t<C>>().rbegin()),
				decltype(std::declval<C>().rend()),
				decltype(std::declval<std::add_const_t<C>>().rend()),
				decltype(std::declval<std::add_const_t<C>>().crbegin()),
				decltype(std::declval<std::add_const_t<C>>().crend())>>
		{
			typedef typename C::reverse_iterator reverse_iterator;
			typedef typename C::const_reverse_iterator const_reverse_iterator;

			static constexpr nBool IsReversibleContainer = true;

			static auto rbegin(C& container)
			{
				return container.rbegin();
			}

			static auto rend(C& container)
			{
				return container.rend();
			}

			static auto crbegin(C const& container)
			{
				return container.crbegin();
			}

			static auto crend(C const& container)
			{
				return container.crend();
			}
		};
	}

	template <typename T>
	class Container
		: public natRefObjImpl<Container<T>>
	{
	public:
		typedef T value_type;
		typedef std::add_lvalue_reference_t<T> reference;
		typedef std::add_lvalue_reference_t<std::add_const_t<T>> const_reference;
		typedef detail_::CommonIterator<T> iterator;
		typedef detail_::CommonIterator<std::add_const_t<T>> const_iterator;
		typedef std::ptrdiff_t difference_type;
		typedef std::size_t size_type;

		// ReversibleContainer
		typedef detail_::CommonIterator<T> reverse_iterator;
		typedef detail_::CommonIterator<std::add_const_t<T>> const_reverse_iterator;

	private:
		struct IContainerWrapper
			: natRefObj
		{
			/*virtual natRefPointer<IContainerWrapper> Clone() const = 0;
			virtual natRefPointer<IContainerWrapper> Move() = 0;*/
			virtual void AssignClone(natRefPointer<IContainerWrapper> const& other) = 0;
			virtual void AssignMove(natRefPointer<IContainerWrapper> const& other) = 0;

			virtual iterator Begin() = 0;
			virtual iterator End() = 0;
			virtual const_iterator CBegin() const = 0;
			virtual const_iterator CEnd() const = 0;

			virtual nBool EqualTo(natRefPointer<IContainerWrapper> const& other) const = 0;
			virtual nBool NotEqualTo(natRefPointer<IContainerWrapper> const& other) const
			{
				return !EqualTo(other);
			}

			virtual void Swap(natRefPointer<IContainerWrapper> const& other) = 0;

			virtual nBool HasSize() const noexcept = 0;
			virtual size_type Size() const
			{
				return std::distance(CBegin(), CEnd());
			}
			virtual size_type MaxSize() const = 0;
			virtual nBool IsEmpty() const = 0;

			// ReversibleContainer
			virtual nBool IsReversibleContainer() const noexcept = 0;
			virtual reverse_iterator RBegin() = 0;
			virtual reverse_iterator REnd() = 0;
			virtual const_reverse_iterator CRBegin() const = 0;
			virtual const_reverse_iterator CREnd() const = 0;
		};

		template <typename C>
		class ContainerWrapper
			: public natRefObjImpl<ContainerWrapper<C>, IContainerWrapper>
		{
		public:
			static_assert(std::numeric_limits<size_type>::max() >= std::numeric_limits<typename C::size_type>::max(), "size_type is not large enough.");
			static_assert(std::numeric_limits<difference_type>::max() >= std::numeric_limits<typename C::difference_type>::max(), "difference_type is not large enough.");

			ContainerWrapper(C& container)
				: m_Container(container)
			{
			}

			C& Get() const noexcept
			{
				return m_Container;
			}

			/*natRefPointer<IContainerWrapper> Clone() const override
			{
				return make_ref<ContainerWrapper>(m_Container);
			}

			natRefPointer<IContainerWrapper> Move() override
			{
				return make_ref<ContainerWrapper>(std::move(m_Container));
			}*/

			void AssignClone(natRefPointer<IContainerWrapper> const& other) override
			{
				const auto realOther = cast(other);
				detail_::ConstContainerConcept<C>::Clone(m_Container, realOther->m_Container);
			}

			void AssignMove(natRefPointer<IContainerWrapper> const& other) override
			{
				const auto realOther = cast(other);
				detail_::ConstContainerConcept<C>::Move(m_Container, std::move(realOther->m_Container));
			}

			iterator Begin() override
			{
				return iterator{ m_Container.begin() };
			}

			iterator End() override
			{
				return iterator{ m_Container.end() };
			}

			const_iterator CBegin() const override
			{
				return const_iterator{ m_Container.cbegin() };
			}

			const_iterator CEnd() const override
			{
				return const_iterator{ m_Container.cend() };
			}

			nBool EqualTo(natRefPointer<IContainerWrapper> const& other) const override
			{
				const auto realOther = cast(other);
				return m_Container == realOther->m_Container;
			}

			nBool NotEqualTo(natRefPointer<IContainerWrapper> const& other) const override
			{
				const auto realOther = cast(other);
				return m_Container != realOther->m_Container;
			}

			void Swap(natRefPointer<IContainerWrapper> const& other) override
			{
				const auto realOther = cast(other);
				detail_::ConstContainerConcept<C>::Swap(m_Container, realOther->m_Container);
			}

			nBool HasSize() const noexcept override
			{
				return detail_::ContainerWithSizeConcept<C>::HasSize;
			}

			size_type Size() const override
			{
				return static_cast<size_type>(detail_::ContainerWithSizeConcept<C>::size(m_Container));
			}

			size_type MaxSize() const override
			{
				return m_Container.max_size();
			}

			nBool IsEmpty() const override
			{
				return m_Container.empty();
			}

			nBool IsReversibleContainer() const noexcept override
			{
				return detail_::ReversibleContainerConcept<C>::IsReversibleContainer;
			}

			reverse_iterator RBegin() override
			{
				return static_cast<reverse_iterator>(detail_::ReversibleContainerConcept<C>::rbegin(m_Container));
			}

			reverse_iterator REnd() override
			{
				return static_cast<reverse_iterator>(detail_::ReversibleContainerConcept<C>::rend(m_Container));
			}

			const_reverse_iterator CRBegin() const
			{
				return static_cast<const_reverse_iterator>(detail_::ReversibleContainerConcept<C>::crbegin(m_Container));
			}

			const_reverse_iterator CREnd() const
			{
				return static_cast<const_reverse_iterator>(detail_::ReversibleContainerConcept<C>::crend(m_Container));
			}

		private:
			C& m_Container;

			static natRefPointer<ContainerWrapper> cast(natRefPointer<IContainerWrapper> const& other)
			{
				const auto realOther = static_cast<natRefPointer<ContainerWrapper>>(other);
				if (!realOther)
				{
					nat_Throw(natErrException, NatErr_InvalidArg, "Require same type."_nv);
				}
				return realOther;
			}
		};

	public:
		Container()
		{
		}

		template <typename C, std::enable_if_t<NonSelf<C, Container>::value, int> = 0>
		Container(C& container)
			: m_Wrapper{ make_ref<ContainerWrapper<C>>(container) }
		{
		}

		Container(Container const& other) = default;

		/*Container(Container&& other) noexcept
			: m_Wrapper{ other.m_Wrapper->Move() }
		{
		}*/

		template <typename C>
		C* GetOriginalContainer() const
		{
			const auto wrapper = static_cast<natRefPointer<ContainerWrapper<C>>>(m_Wrapper);
			if (!wrapper)
			{
				return nullptr;
			}

			return &(wrapper->Get());
		}

		Container& operator=(Container const& other)
		{
			m_Wrapper->AssignClone(other.m_Wrapper);
			return *this;
		}

		Container& operator=(Container&& other) noexcept
		{
			m_Wrapper->AssignMove(other.m_Wrapper);
			return *this;
		}

		iterator begin()
		{
			return m_Wrapper->Begin();
		}

		const_iterator begin() const
		{
			return m_Wrapper->CBegin();
		}

		iterator end()
		{
			return m_Wrapper->End();
		}

		const_iterator end() const
		{
			return m_Wrapper->CEnd();
		}

		const_iterator cbegin() const
		{
			return m_Wrapper->CBegin();
		}

		const_iterator cend() const
		{
			return m_Wrapper->CEnd();
		}

		nBool operator==(Container const& other)
		{
			return m_Wrapper->EqualTo(other.m_Wrapper);
		}

		nBool operator!=(Container const& other)
		{
			return m_Wrapper->NotEqualTo(other.m_Wrapper);
		}

		void swap(Container& other) noexcept
		{
			m_Wrapper->Swap(other.m_Wrapper);
		}

		nBool HasSize() const noexcept
		{
			return m_Wrapper->HasSize();
		}

		size_type size() const
		{
			return m_Wrapper->Size();
		}
		
		size_type max_size() const
		{
			return m_Wrapper->MaxSize();
		}

		nBool empty() const
		{
			return m_Wrapper->IsEmpty();
		}

		// ReversibleContainer
		nBool IsReversibleContainer() const noexcept
		{
			return m_Wrapper->IsReversibleContainer();
		}

		reverse_iterator rbegin()
		{
			return m_Wrapper->RBegin();
		}

		const_reverse_iterator rbegin() const
		{
			return m_Wrapper->CRBegin();
		}

		reverse_iterator rend()
		{
			return m_Wrapper->REnd();
		}

		const_reverse_iterator rend() const
		{
			return m_Wrapper->CREnd();
		}

		const_reverse_iterator crbegin() const
		{
			return m_Wrapper->CRBegin();
		}

		const_reverse_iterator crend() const
		{
			return m_Wrapper->CREnd();
		}

	private:
		natRefPointer<IContainerWrapper> m_Wrapper;
	};

	template <typename T>
	void swap(Container<T>& a, Container<T>& b) noexcept
	{
		a.swap(b);
	}
}

#ifdef _MSC_VER
#pragma pop_macro("max")
#endif
