////////////////////////////////////////////////////////////////////////////////
///	@file	natRefObj.h
///	@brief	引用计数对象相关
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natType.h"
#include <cassert>

#include <mutex>
#include <atomic>
#include <memory>

#ifdef TraceRefObj
#include "natUtil.h"
#include <typeinfo>
#endif

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	引用计数接口
	////////////////////////////////////////////////////////////////////////////////
	struct natRefObj
	{
		virtual ~natRefObj() = default;

		virtual size_t GetRefCount() const volatile noexcept = 0;
		virtual nBool TryAddRef() const volatile = 0;
		virtual void AddRef() const volatile = 0;
		virtual nBool Release() const volatile = 0;
	};

	template <typename T>
	class natRefPointer;

	template <typename T>
	class natWeakRefPointer;

	namespace detail_
	{
		template <typename Dst, typename Src, typename = Dst>
		struct static_cast_or_dynamic_cast_helper
		{
			static Dst Do(Src& src)
			{
				return dynamic_cast<Dst>(static_cast<Src&&>(src));
			}
		};

		template <typename Dst, typename Src>
		struct static_cast_or_dynamic_cast_helper<Dst, Src, decltype(static_cast<Dst>(std::declval<Src>()))>
		{
			static constexpr Dst Do(Src& src) noexcept
			{
				return static_cast<Dst>(static_cast<Src&&>(src));
			}
		};

		template <typename Dst, typename Src>
		constexpr Dst static_cast_or_dynamic_cast(Src&& src)
		{
			return static_cast_or_dynamic_cast_helper<Dst, Src>::Do(src);
		}

		template <typename Base = natRefObj>
		class RefCountBase
			: public Base
		{
		public:
			nBool IsUnique() const volatile noexcept
			{
				return GetRefCount() == 1;
			}

			virtual size_t GetRefCount() const volatile noexcept
			{
				return m_RefCount.load(std::memory_order_relaxed);
			}

			virtual nBool TryAddRef() const volatile
			{
				auto oldValue = m_RefCount.load(std::memory_order_relaxed);
				assert(static_cast<std::ptrdiff_t>(oldValue) >= 0);
				do
				{
					if (oldValue == 0)
					{
						return false;
					}
				} while (!m_RefCount.compare_exchange_strong(oldValue, oldValue + 1, std::memory_order_relaxed, std::memory_order_relaxed));
				return true;
			}

			virtual void AddRef() const volatile
			{
				assert(static_cast<std::ptrdiff_t>(m_RefCount.load(std::memory_order_relaxed)) > 0);
				m_RefCount.fetch_add(1, std::memory_order_relaxed);
			}

			virtual nBool Release() const volatile
			{
				assert(static_cast<std::ptrdiff_t>(m_RefCount.load(std::memory_order_relaxed)) > 0);
				return m_RefCount.fetch_sub(1, std::memory_order_relaxed) == 1;
			}

		protected:
			constexpr RefCountBase() noexcept
				: m_RefCount(1)
			{
			}
			constexpr RefCountBase(RefCountBase const&) noexcept
				: RefCountBase()
			{
			}
			RefCountBase& operator=(RefCountBase const&) noexcept
			{
				return *this;
			}

			~RefCountBase()
			{
				assert(RefCountBase::GetRefCount() <= 1);
			}

		private:
			mutable std::atomic<size_t> m_RefCount;
		};

		template <typename Owner>
		class WeakRefView final
			: public RefCountBase<natRefObj>
		{
		public:
			constexpr explicit WeakRefView(std::add_pointer_t<Owner> owner) noexcept
				: m_Mutex{}, m_Owner{ owner }
			{
			}

			nBool IsOwnerAlive() const
			{
				const std::lock_guard<std::mutex> lock{ m_Mutex };
				const auto owner = m_Owner;
				return owner && static_cast<const volatile RefCountBase*>(owner)->GetRefCount() > 0;
			}

			void ClearOwner()
			{
				const std::lock_guard<std::mutex> lock{ m_Mutex };
				m_Owner = nullptr;
			}

			template <typename T>
			natRefPointer<T> LockOwner() const
			{
				const std::lock_guard<std::mutex> lock{ m_Mutex };
				const auto other = static_cast_or_dynamic_cast<std::add_pointer_t<T>>(m_Owner);
				if (!other)
				{
					return {};
				}

				if (!static_cast<std::add_cv_t<decltype(m_Owner)>>(m_Owner)->TryAddRef())
				{
					return {};
				}

				return { other };
			}

		private:
			mutable std::mutex m_Mutex;
			std::add_pointer_t<Owner> m_Owner;
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	引用计数实现
	///	@note	使用模板防止菱形继承
	////////////////////////////////////////////////////////////////////////////////
	template <typename T, typename B = natRefObj>
	class natRefObjImpl
		: public detail_::RefCountBase<B>
	{
		static_assert(std::is_base_of<natRefObj, B>::value, "B should inherit from natRefObj.");

		template <typename T_>
		friend class natWeakRefPointer;

		typedef detail_::RefCountBase<B> Base;

		template <typename T_, typename ...Arg>
		friend NATINLINE natRefPointer<T_> make_ref(Arg &&... args);

		struct DefaultDeleter
		{
			void operator()(T* ptr) const noexcept
			{
				delete ptr;
			}
		};

	protected:
		void* operator new(size_t size)
		{
			return ::operator new(size);
		}

		void operator delete(void* ptr)
		{
			::operator delete(ptr);
		}

	public:
		typedef std::function<void(T*)> SelfDeleter;
		typedef detail_::WeakRefView<natRefObjImpl> WeakRefView;

		constexpr natRefObjImpl(SelfDeleter deleter = {}) noexcept
			: m_View{ nullptr }, m_Deleter{ std::move(deleter) }
		{
#ifdef TraceRefObj
			OutputDebugString(natUtil::FormatString("Type %s created at (%p)\n"_nv, nStringView{ typeid(*this).name() }, this).c_str());
#endif
		}
		constexpr natRefObjImpl(natRefObjImpl const&) noexcept
			: m_View{ nullptr }
		{
		}
		natRefObjImpl& operator=(natRefObjImpl const&) noexcept
		{
			return *this;
		}

		virtual ~natRefObjImpl()
		{
#ifdef TraceRefObj
			OutputDebugString(natUtil::FormatString("Type %s destroyed at (%p)\n"_nv, nStringView{ typeid(*this).name() }, this).c_str());
#endif
			const auto view = m_View.load(std::memory_order_consume);
			if (view)
			{
				if (static_cast<const volatile detail_::RefCountBase<natRefObj>*>(view)->Release())
				{
					delete view;
				}
				else
				{
					view->ClearOwner();
				}
			}
		}

		nBool Release() const volatile override
		{
			const auto result = Base::Release();
			const SelfDeleter& deleter = const_cast<const SelfDeleter&>(m_Deleter);
			if (result && deleter)
			{
				deleter(static_cast<T*>(const_cast<natRefObjImpl*>(this)));
			}
			return result;
		}

		void SetDeleter(SelfDeleter deleter = DefaultDeleter{})
		{
			m_Deleter = std::move(deleter);
		}

		template <typename U>
		natRefPointer<U> ForkRef() noexcept
		{
			return forkRefImpl<U>(this);
		}

		template <typename U>
		natRefPointer<const U> ForkRef() const noexcept
		{
			return forkRefImpl<const U>(this);
		}

		template <typename U>
		natRefPointer<volatile U> ForkRef() volatile noexcept
		{
			return forkRefImpl<volatile U>(this);
		}

		template <typename U>
		natRefPointer<const volatile U> ForkRef() const volatile noexcept
		{
			return forkRefImpl<const volatile U>(this);
		}

		template <typename U>
		natWeakRefPointer<U> ForkWeakRef() noexcept
		{
			return forkWeakRefImpl<U>(this);
		}

		template <typename U>
		natWeakRefPointer<const U> ForkWeakRef() const noexcept
		{
			return forkWeakRefImpl<const U>(this);
		}

		template <typename U>
		natWeakRefPointer<volatile U> ForkWeakRef() volatile noexcept
		{
			return forkWeakRefImpl<volatile U>(this);
		}

		template <typename U>
		natWeakRefPointer<const volatile U> ForkWeakRef() const volatile noexcept
		{
			return forkWeakRefImpl<const volatile U>(this);
		}

	private:
		template <typename CVU, typename CVT>
		static natRefPointer<CVU> forkRefImpl(CVT* pThis)
		{
			const auto other = detail_::static_cast_or_dynamic_cast<CVU*>(pThis);
			if (!other)
			{
				return {};
			}

			return natRefPointer<CVU>{ other };
		}

		template <typename CVU, typename CVT>
		static natWeakRefPointer<CVU> forkWeakRefImpl(CVT* pThis)
		{
			const auto other = detail_::static_cast_or_dynamic_cast<CVU*>(pThis);
			if (!other)
			{
				return {};
			}

			return natWeakRefPointer<CVU>{ other };
		}

		WeakRefView* createWeakRefView() const volatile
		{
			auto view = m_View.load(std::memory_order_consume);
			if (!view)
			{
				const auto newView = new WeakRefView(const_cast<natRefObjImpl*>(this));
				if (m_View.compare_exchange_strong(view, newView, std::memory_order_release, std::memory_order_consume))
				{
					view = newView;
				}
				else
				{
					delete newView;
				}
			}
			return view;
		}

		mutable std::atomic<WeakRefView*> m_View;
		SelfDeleter m_Deleter;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	强引用指针实现
	///	@note	仅能用于引用计数对象
	////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	class natRefPointer final
	{
	public:
		constexpr natRefPointer(std::nullptr_t = nullptr) noexcept
			: m_pPointer(nullptr)
		{
		}

		constexpr explicit natRefPointer(T* ptr) noexcept
			: m_pPointer(ptr)
		{
			if (m_pPointer)
			{
				static_cast<const volatile natRefObj*>(m_pPointer)->AddRef();
			}
		}

		constexpr natRefPointer(natRefPointer const& other) noexcept
			: m_pPointer(other.m_pPointer)
		{
			if (m_pPointer)
			{
				m_pPointer->AddRef();
			}
		}

		constexpr natRefPointer(natRefPointer && other) noexcept
			: m_pPointer(other.m_pPointer)
		{
			other.m_pPointer = nullptr;
		}

		~natRefPointer()
		{
			SafeRelease(m_pPointer);
		}

		void Reset(std::nullptr_t = nullptr) noexcept
		{
			natRefPointer{}.swap(*this);
		}

		void Reset(T* ptr) noexcept
		{
			natRefPointer{ ptr }.swap(*this);
		}

		template <typename U>
		void Reset(natRefPointer<U> const& other) noexcept
		{
			natRefPointer{ other }.swap(*this);
		}

		template <typename U>
		void Reset(natRefPointer<U> && other) noexcept
		{
			natRefPointer{ other }.swap(*this);
		}

		void Reset(natRefPointer const& other) noexcept
		{
			natRefPointer{ other }.swap(*this);
		}

		void Reset(natRefPointer && other) noexcept
		{
			natRefPointer{ std::move(other) }.swap(*this);
		}

		template <typename U>
		nBool operator==(natRefPointer<U> const& other) const noexcept
		{
			return m_pPointer == other.Get();
		}

		template <typename U>
		nBool operator!=(natRefPointer<U> const& other) const noexcept
		{
			return m_pPointer != other.Get();
		}

		template <typename U>
		nBool operator<(natRefPointer<U> const& other) const noexcept
		{
			return m_pPointer < other.Get();
		}

		template <typename U>
		nBool operator>(natRefPointer<U> const& other) const noexcept
		{
			return m_pPointer > other.Get();
		}

		template <typename U>
		nBool operator<=(natRefPointer<U> const& other) const noexcept
		{
			return m_pPointer <= other.Get();
		}

		template <typename U>
		nBool operator>=(natRefPointer<U> const& other) const noexcept
		{
			return m_pPointer >= other.Get();
		}

		natRefPointer& operator=(natRefPointer const& other)&
		{
			return Reset(other.m_pPointer);
		}

		natRefPointer& operator=(natRefPointer && other)& noexcept
		{
			std::swap(m_pPointer, other.m_pPointer);
			return *this;
		}

		natRefPointer& operator=(std::nullptr_t)
		{
			SafeRelease(m_pPointer);
			return *this;
		}

		T* operator->() const noexcept
		{
			assert(m_pPointer && "m_pPointer is nullptr.");
			return m_pPointer;
		}

		T& operator*() const noexcept
		{
			assert(m_pPointer && "m_pPointer is nullptr.");
			return *m_pPointer;
		}

		T** operator&() noexcept
		{
			Reset(nullptr);
			return &m_pPointer;
		}

		explicit operator nBool() const noexcept
		{
			return m_pPointer;
		}

		operator T*() const noexcept
		{
			return m_pPointer;
		}

		T* Get() const noexcept
		{
			return m_pPointer;
		}

		size_t GetRefCount() const noexcept
		{
			const auto ptr = m_pPointer;
			if (!ptr)
			{
				return 0;
			}
			return static_cast<const volatile natRefObj*>(ptr)->GetRefCount();
		}

		void swap(natRefPointer& other) noexcept
		{
			const auto ptr = m_pPointer;
			m_pPointer = other.m_pPointer;
			other.m_pPointer = ptr;
		}

		template <typename P>
		operator natRefPointer<P>() const;

	private:
		T* m_pPointer;
	};

	template <typename T, typename ...Arg>
	NATINLINE natRefPointer<T> make_ref(Arg &&... args)
	{
		const auto pRefObj = new T(std::forward<Arg>(args)...);
		pRefObj->SetDeleter();
		natRefPointer<T> Ret(pRefObj);
		pRefObj->Release();
		return std::move(Ret);
	}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	弱引用指针实现
	///	@note	仅能用于引用计数对象
	////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	class natWeakRefPointer
	{
		template <typename>
		friend class natWeakRefPointer;

		typedef typename T::WeakRefView WeakRefView;

		static WeakRefView* GetViewFrom(const volatile T* item)
		{
			if (!item)
			{
				return nullptr;
			}
			const auto view = item->createWeakRefView();
			view->AddRef();
			return view;
		}

	public:
		typedef std::add_pointer_t<T> pointer;

		constexpr natWeakRefPointer(std::nullptr_t = nullptr) noexcept
			: m_View{}
		{
		}

		explicit natWeakRefPointer(pointer ptr)
			: natWeakRefPointer(GetViewFrom(ptr))
		{
		}

		template <typename U, std::enable_if_t<std::is_convertible<typename natWeakRefPointer<U>::pointer, pointer>::value, int> = 0>
		natWeakRefPointer(natRefPointer<U> const& other) noexcept
			: natWeakRefPointer(GetViewFrom(other.Get()))
		{
		}

		template <typename U, std::enable_if_t<std::is_convertible<typename natWeakRefPointer<U>::pointer, pointer>::value, int> = 0>
		natWeakRefPointer(natWeakRefPointer<U> const& other) noexcept
			:natWeakRefPointer(other.fork())
		{
		}

		template <typename U, std::enable_if_t<std::is_convertible<typename natWeakRefPointer<U>::pointer, pointer>::value, int> = 0>
		natWeakRefPointer(natWeakRefPointer<U> && other) noexcept
			: natWeakRefPointer(other.release())
		{
		}

		natWeakRefPointer(natWeakRefPointer const& other) noexcept
			: natWeakRefPointer(other.fork())
		{
		}

		natWeakRefPointer(natWeakRefPointer && other) noexcept
			: natWeakRefPointer(other.release())
		{
		}

		~natWeakRefPointer()
		{
			SafeRelease(m_View);
		}

		natWeakRefPointer& operator=(natWeakRefPointer const& other) noexcept
		{
			Reset(other);
			return *this;
		}

		natWeakRefPointer& operator=(natWeakRefPointer && other) noexcept
		{
			Reset(std::move(other));
			return *this;
		}

		nBool IsExpired() const noexcept
		{
			const auto view = m_View;
			if (!view)
			{
				return true;
			}
			return !view->IsOwnerAlive();
		}

		size_t WeakCount() const noexcept
		{
			const auto view = m_View;
			if (!view)
			{
				return 0;
			}
			return view->GetRefCount() - 1;	// 本体自带1个对WeakRefView的引用
		}

		template <typename U = T>
		natRefPointer<U> Lock() const noexcept
		{
			const auto view = m_View;
			if (!view)
			{
				return {};
			}
			return view->template LockOwner<U>();
		}

		void Reset(std::nullptr_t = nullptr) noexcept
		{
			natWeakRefPointer{}.swap(*this);
		}

		void Reset(pointer ptr) noexcept
		{
			natWeakRefPointer{ ptr }.swap(*this);
		}

		template <typename U>
		void Reset(natRefPointer<U> const& other) noexcept
		{
			natWeakRefPointer{ other }.swap(*this);
		}

		template <typename U>
		void Reset(natWeakRefPointer<U> const& other) noexcept
		{
			natWeakRefPointer{ other }.swap(*this);
		}

		template <typename U>
		void Reset(natWeakRefPointer<U> && other) noexcept
		{
			natWeakRefPointer{ std::move(other) }.swap(*this);
		}

		void Reset(natWeakRefPointer const& other) noexcept
		{
			natWeakRefPointer{ other }.swap(*this);
		}

		void Reset(natWeakRefPointer && other) noexcept
		{
			natWeakRefPointer{ std::move(other) }.swap(*this);
		}

		void swap(natWeakRefPointer& other) noexcept
		{
			const auto view = m_View;
			m_View = other.m_View;
			other.m_View = view;
		}

		template <typename U>
		nBool operator==(natWeakRefPointer<U> const& other) const noexcept
		{
			return m_View == other.m_View;
		}

		template <typename U>
		nBool operator!=(natWeakRefPointer<U> const& other) const noexcept
		{
			return m_View != other.m_View;
		}

		template <typename U>
		nBool operator<(natWeakRefPointer<U> const& other) const noexcept
		{
			return m_View < other.m_View;
		}

		template <typename U>
		nBool operator>(natWeakRefPointer<U> const& other) const noexcept
		{
			return m_View > other.m_View;
		}

		template <typename U>
		nBool operator<=(natWeakRefPointer<U> const& other) const noexcept
		{
			return m_View <= other.m_View;
		}

		template <typename U>
		nBool operator>=(natWeakRefPointer<U> const& other) const noexcept
		{
			return m_View >= other.m_View;
		}

		explicit operator nBool() const noexcept
		{
			return m_View;
		}

		// Workaround
		size_t GetHashCode() const noexcept
		{
			return std::hash<WeakRefView*>{}(m_View);
		}

	private:
		WeakRefView* m_View;

		constexpr explicit natWeakRefPointer(WeakRefView* view) noexcept
			: m_View{ view }
		{
		}

		WeakRefView* fork() const
		{
			const auto view = m_View;
			if (view)
			{
				static_cast<const volatile natRefObj*>(m_View)->AddRef();
			}
			return view;
		}

		WeakRefView* release() noexcept
		{
			return std::exchange(m_View, nullptr);
		}
	};
}

template <typename T>
NATINLINE void swap(NatsuLib::natRefPointer<T>& lhs, NatsuLib::natRefPointer<T>& rhs) noexcept
{
	lhs.swap(rhs);
}

template <typename T>
NATINLINE void swap(NatsuLib::natWeakRefPointer<T>& lhs, NatsuLib::natWeakRefPointer<T>& rhs) noexcept
{
	lhs.swap(rhs);
}

namespace std
{
	template <typename T>
	struct hash<NatsuLib::natRefPointer<T>>
	{
		size_t operator()(NatsuLib::natRefPointer<T> const& ptr) const
		{
			return hash<T*>{}(ptr.Get());
		}
	};

	template <typename T>
	struct hash<NatsuLib::natWeakRefPointer<T>>
	{
		size_t operator()(NatsuLib::natWeakRefPointer<T> const& ptr) const
		{
			return ptr.GetHashCode();
		}
	};
}

#include "natException.h"

namespace NatsuLib
{
	template <typename T>
	template <typename P>
	natRefPointer<T>::operator natRefPointer<P>() const
	{
		if (!m_pPointer)
		{
			return {};
		}

		const auto pTarget = detail_::static_cast_or_dynamic_cast<P*>(m_pPointer);
		if (pTarget)
		{
			return natRefPointer<P> { pTarget };
		}

		nat_Throw(natException, "Type P cannot be converted to T."_nv);
	}
}
