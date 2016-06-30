////////////////////////////////////////////////////////////////////////////////
///	@file	natRefObj.h
///	@brief	引用计数对象相关
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natType.h"
#include <Windows.h>

#ifdef TRACEREFOBJ
#ifdef _DEBUG
#include "natUtil.h"
#include <typeinfo>
#endif // _DEBUG
#endif // TRACEREFOBJ

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
		natRefObjImpl()
			: m_cRef(1u)
		{
#ifdef TRACEREFOBJ
#ifdef _DEBUG
			OutputDebugString(natUtil::FormatString(_T("Type %s Create at (%p)\n"), natUtil::C2Wstr(typeid(*this).name()).c_str(), this).c_str());
#endif // _DEBUG
#endif
		}
		virtual ~natRefObjImpl()
		{
#ifdef TRACEREFOBJ
#ifdef _DEBUG
			OutputDebugString(natUtil::FormatString(_T("Type %s Destroy at (%p)\n"), natUtil::C2Wstr(typeid(*this).name()).c_str(), this).c_str());
#endif // _DEBUG
#endif
		}

		virtual void AddRef()
		{
			InterlockedIncrement(&m_cRef);
		}

		virtual void Release()
		{
			auto tRet = InterlockedDecrement(&m_cRef);
			if (tRet == 0u)
			{
				delete this;
			}
		}

	private:
		nuInt m_cRef;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	智能指针实现
	///	@note	仅能用于引用计数对象
	////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	class natRefPointer final
	{
	public:
		natRefPointer()
			: m_pPointer(nullptr)
		{
		}

		explicit natRefPointer(T* ptr)
			: m_pPointer(ptr)
		{
			if (m_pPointer)
			{
				m_pPointer->AddRef();
			}
		}

		natRefPointer(natRefPointer const& other)
			: m_pPointer(other.m_pPointer)
		{
			if (m_pPointer)
			{
				m_pPointer->AddRef();
			}
		}

		natRefPointer(natRefPointer && other)
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
			}

			return *this;
		}

		bool operator==(natRefPointer const& other) const
		{
			return (m_pPointer == other.m_pPointer);
		}

		natRefPointer& operator=(natRefPointer const& other)&
		{
			if (m_pPointer != other.m_pPointer)
			{
				SafeRelease(m_pPointer);
				m_pPointer = other.m_pPointer;
				if (m_pPointer)
				{
					m_pPointer->AddRef();
				}
			}

			return *this;
		}

		natRefPointer& operator=(natRefPointer && other)&
		{
			std::swap(m_pPointer, other.m_pPointer);
			return *this;
		}

		natRefPointer& operator=(nullptr_t)
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
		return Ret;
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
