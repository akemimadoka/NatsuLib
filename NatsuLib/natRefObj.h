////////////////////////////////////////////////////////////////////////////////
///	@file	natRefObj.h
///	@brief	引用计数对象相关
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natType.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <atomic>
#endif

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

		virtual void AddRef() = 0;
		virtual void Release() = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	引用计数实现
	///	@note	使用模板防止菱形继承
	////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	class natRefObjImpl
		: public T
	{
	public:
		constexpr natRefObjImpl() noexcept
			: m_cRef(1u)
		{
#ifdef TraceRefObj
			OutputDebugString(natUtil::FormatString(_T("Type %s created at (%p)\n"), natUtil::C2Wstr(typeid(*this).name()).c_str(), this).c_str());
#endif
		}
		virtual ~natRefObjImpl()
		{
#ifdef TraceRefObj
			OutputDebugString(natUtil::FormatString(_T("Type %s destroyed at (%p)\n"), natUtil::C2Wstr(typeid(*this).name()).c_str(), this).c_str());
#endif
		}

		void AddRef() override
		{
#ifdef _WIN32
			InterlockedIncrement(&m_cRef);
#else
			++m_cRef;
#endif
		}

		void Release() override
		{
			auto tRet =
#ifdef _WIN32
				InterlockedDecrement(&m_cRef);
#else
				--m_cRef;
#endif
				
			if (tRet == 0u)
			{
				delete this;
			}
		}

	private:
#ifdef _WIN32
		nuInt m_cRef;
#else
		std::atomic<nuInt> m_cRef;
#endif
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	智能指针实现
	///	@note	仅能用于引用计数对象
	////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	class natRefPointer final
	{
	public:
		constexpr natRefPointer() noexcept
			: m_pPointer(nullptr)
		{
		}

		constexpr explicit natRefPointer(T* ptr) noexcept
			: m_pPointer(ptr)
		{
			if (m_pPointer)
			{
				m_pPointer->AddRef();
			}
		}

		constexpr natRefPointer(std::nullptr_t) noexcept
			: m_pPointer(nullptr)
		{
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

		natRefPointer& RawSet(T* ptr)
		{
			if (ptr != m_pPointer)
			{
				SafeRelease(m_pPointer);
				m_pPointer = ptr;
				if (m_pPointer)
				{
					m_pPointer->AddRef();
				}
			}

			return *this;
		}

		bool operator==(natRefPointer const& other) const
		{
			return (m_pPointer == other.m_pPointer);
		}

		natRefPointer& operator=(natRefPointer const& other)&
		{
			return RawSet(other.m_pPointer);
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

		T* operator->() const
		{
			return m_pPointer;
		}

		T& operator*() const
		{
			return *m_pPointer;
		}

		T** operator&()
		{
			RawSet(nullptr);
			return &m_pPointer;
		}

		operator T*() const
		{
			return m_pPointer;
		}

		T* Get() const
		{
			return m_pPointer;
		}

		template <typename P>
		operator natRefPointer<P>() const
		{
			return natRefPointer<P>(dynamic_cast<P*>(m_pPointer));
		}
	private:
		T* m_pPointer;
	};

	template <typename T, typename ...Arg>
	NATINLINE natRefPointer<T> make_ref(Arg &&... args)
	{
		T* pRefObj = new T(std::forward<Arg>(args)...);
		natRefPointer<T> Ret(pRefObj);
		SafeRelease(pRefObj);
		return std::move(Ret);
	}
}

namespace std
{
	template <typename T>
	struct hash<NatsuLib::natRefPointer<T>>
	{
		size_t operator()(NatsuLib::natRefPointer<T> const& _Keyval) const
		{
			return hash<T*>()(_Keyval.Get());
		}
	};
}

