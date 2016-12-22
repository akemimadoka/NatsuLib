////////////////////////////////////////////////////////////////////////////////
///	@file	natException.h
///	@brief	异常相关头文件
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natType.h"
#include "natString.h"
#include <chrono>
#ifdef _WIN32
#include <Windows.h>
#endif
#ifdef EnableExceptionStackTrace
#include "natStackWalker.h"
#endif

namespace NatsuLib
{
	namespace natUtil
	{
		template <typename... Args>
		nString FormatString(nStrView const& Str, Args&&... args);
	}

	namespace detail_
	{
		struct natExceptionStorage
		{
			natExceptionStorage(std::exception_ptr nestedException, std::chrono::system_clock::time_point const& time, nString const& file, nuInt line, nString const& src, nString const& desc);

			std::exception_ptr m_NestedException;
			std::chrono::system_clock::time_point m_Time;
			nString m_File;
			nuInt m_Line;
			nString m_Source;
			nString m_Description;
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	NatsuLib异常基类
	///	@note	异常由此类派生，请勿使用可能抛出异常的代码
	////////////////////////////////////////////////////////////////////////////////
	class natException
		: protected detail_::natExceptionStorage, public virtual std::exception
	{
	public:
		typedef detail_::natExceptionStorage Storage;

		template <typename... Args>
		natException(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args) noexcept
			: Storage{ nestedException, std::chrono::system_clock::now(), File, Line, Src, natUtil::FormatString(Desc, std::forward<Args>(args)...) }
#ifdef EnableExceptionStackTrace
			, m_StackWalker()
#endif
		{
#ifdef EnableExceptionStackTrace
			m_StackWalker.CaptureStack();
#endif
		}

		template <typename... Args>
		natException(nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args) noexcept
			: natException(std::exception_ptr{}, Src, File, Line, Desc, std::forward<Args>(args)...)
		{
		}

		virtual ~natException();

		std::chrono::system_clock::time_point GetTime() const noexcept;
		nStrView GetFile() const noexcept;
		nuInt GetLine() const noexcept;
		nStrView GetSource() const noexcept;
		nStrView GetDesc() const noexcept;
		std::exception_ptr GetNestedException() const noexcept;

#ifdef EnableExceptionStackTrace
		natStackWalker const& GetStackWalker() const noexcept;
#endif

		ncStr what() const noexcept override;

	protected:
#ifdef EnableExceptionStackTrace
		natStackWalker m_StackWalker;
#endif
	};

#ifdef _WIN32
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	NatsuLib WinAPI调用异常
	///	@note	可以自动附加LastErr信息
	////////////////////////////////////////////////////////////////////////////////
	class natWinException
		: public natException
	{
	public:
		template <typename... Args>
		natWinException(nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args) noexcept
			: natWinException(Src, File, Line, GetLastError(), Desc, std::forward<Args>(args)...)
		{
		}

		template <typename... Args>
		natWinException(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args) noexcept
			: natWinException(nestedException, Src, File, Line, GetLastError(), Desc, std::forward<Args>(args)...)
		{
		}

		template <typename... Args>
		natWinException(nStrView Src, nStrView File, nuInt Line, DWORD LastErr, nStrView Desc, Args&&... args) noexcept
			: natException(Src, File, Line, Desc, std::forward<Args>(args)...), m_LastErr(LastErr), m_ErrMsg()
		{
			m_Description.Append(natUtil::FormatString(" (LastErr = {0})"_nv, m_LastErr));
		}

		template <typename... Args>
		natWinException(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line, DWORD LastErr, nStrView Desc, Args&&... args) noexcept
			: natException(nestedException, Src, File, Line, Desc, std::forward<Args>(args)...), m_LastErr(LastErr), m_ErrMsg()
		{
			m_Description.Append(natUtil::FormatString(" (LastErr = {0})"_nv, m_LastErr));
		}

		DWORD GetErrNo() const noexcept;
		nStrView GetErrMsg() const noexcept;

	private:
		DWORD m_LastErr;
		mutable nString m_ErrMsg;
	};
#endif

	class natErrException
		: public natException
	{
	public:
		template <typename... Args>
		natErrException(nStrView Src, nStrView File, nuInt Line, NatErr ErrNo, nStrView Desc, Args&&... args) noexcept
			: natException(Src, File, Line, Desc, std::forward<Args>(args)...), m_Errno(ErrNo)
		{
			m_Description.Append(natUtil::FormatString(" (Errno = {0})"_nv, m_Errno));
		}

		template <typename... Args>
		natErrException(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line, NatErr ErrNo, nStrView Desc, Args&&... args) noexcept
			: natException(nestedException, Src, File, Line, Desc, std::forward<Args>(args)...), m_Errno(ErrNo)
		{
			m_Description.Append(natUtil::FormatString(" (Errno = {0})"_nv, m_Errno));
		}

		NatErr GetErrNo() const noexcept;
		nStrView GetErrMsg() const noexcept;

	private:
		NatErr m_Errno;

		static NATINLINE nStrView GetErrDescription(NatErr Errno)
		{
			switch (Errno)
			{
			case NatErr_Interrupted:
				return "Interrupted"_nv;
			case NatErr_OK:
				return "Success"_nv;
			case NatErr_Unknown:
				return "Unknown error"_nv;
			case NatErr_IllegalState:
				return "Illegal state"_nv;
			case NatErr_InvalidArg:
				return "Invalid argument"_nv;
			case NatErr_InternalErr:
				return "Internal error"_nv;
			case NatErr_OutOfRange:
				return "Out of range"_nv;
			case NatErr_NotImpl:
				return "Not implemented"_nv;
			case NatErr_NotSupport:
				return "Not supported"_nv;
			case NatErr_Duplicated:
				return "Duplicated"_nv;
			case NatErr_NotFound:
				return "Not found"_nv;
			default:
				return "No description"_nv;
			}
		}
	};

#define DeclareException(ExceptionClass, ExtendException, DefaultDescription) \
class ExceptionClass : public ExtendException\
{\
public:\
	typedef ExtendException BaseException;\
	ExceptionClass(nStrView Src, nStrView File, nuInt Line)\
		: BaseException(Src, File, Line, DefaultDescription)\
	{\
	}\
	ExceptionClass(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line)\
		: BaseException(nestedException, Src, File, Line, DefaultDescription)\
	{\
	}\
	template <typename... Args>\
	ExceptionClass(nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args)\
		: BaseException(Src, File, Line, Desc, std::forward<Args>(args)...)\
	{\
	}\
	template <typename... Args>\
	ExceptionClass(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args)\
		: BaseException(nestedException, Src, File, Line, Desc, std::forward<Args>(args)...)\
	{\
	}\
}

	DeclareException(OutOfRange, natException, "Out of range."_nv);
	DeclareException(MemoryAllocFail, natException, "Failed to allocate memory."_nv);
}

#define nat_Throw(ExceptionClass, ...) do { throw ExceptionClass(nStrView{ __FUNCTION__ }, NV(__FILE__), __LINE__, __VA_ARGS__); } while (false)
#define nat_ThrowWithNested(ExceptionClass, ...) do { throw ExceptionClass(std::current_exception(), nStrView{ __FUNCTION__ }, NV(__FILE__), __LINE__, __VA_ARGS__); } while (false)
#define nat_ThrowIfFailed(Expression, ...) do { nResult result; if (NATFAIL(result = (Expression))) nat_Throw(natErrException, static_cast<NatErr>(result), __VA_ARGS__); } while (false)

#include "natStringUtil.h"
