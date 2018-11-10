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
#ifdef NATSULIB_ENABLE_EXCEPTION_STACK_TRACE
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
			natExceptionStorage(std::exception_ptr nestedException, std::chrono::system_clock::time_point const& time, nString file, nuInt line, nString src, nString desc);

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
			: Storage{ std::move(nestedException), std::chrono::system_clock::now(), File, Line, Src, natUtil::FormatString(Desc, std::forward<Args>(args)...) }
		{
#ifdef NATSULIB_ENABLE_EXCEPTION_STACK_TRACE
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

#ifdef NATSULIB_ENABLE_EXCEPTION_STACK_TRACE
		natStackWalker const& GetStackWalker() const noexcept;
#endif

		[[noreturn]] void Throw() const;
		[[noreturn]] void ThrowWithNested() const;

		ncStr what() const noexcept override;

	protected:
#ifdef NATSULIB_ENABLE_EXCEPTION_STACK_TRACE
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
			m_Description.Append(natUtil::FormatString(u8" (LastErr = {0}, Message = {1})"_nv, m_LastErr, GetErrMsg()));
		}

		template <typename... Args>
		natWinException(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line, DWORD LastErr, nStrView Desc, Args&&... args) noexcept
			: natException(nestedException, Src, File, Line, Desc, std::forward<Args>(args)...), m_LastErr(LastErr), m_ErrMsg()
		{
			m_Description.Append(natUtil::FormatString(u8" (LastErr = {0}, Message = {1})"_nv, m_LastErr, GetErrMsg()));
		}

		~natWinException();

		DWORD GetErrNo() const noexcept;
		nStrView GetErrMsg() const noexcept;

	private:
		DWORD m_LastErr;
		mutable nString m_ErrMsg;
	};
#endif

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	携带 NatErr 信息的异常
	////////////////////////////////////////////////////////////////////////////////
	class natErrException
		: public natException
	{
	public:
		template <typename... Args>
		natErrException(nStrView Src, nStrView File, nuInt Line, NatErr ErrNo, nStrView Desc, Args&&... args) noexcept
			: natException(Src, File, Line, Desc, std::forward<Args>(args)...), m_Errno(ErrNo)
		{
			m_Description.Append(natUtil::FormatString(u8" (Errno = {0}, Message = {1})"_nv, m_Errno, GetErrMsg()));
		}

		template <typename... Args>
		natErrException(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line, NatErr ErrNo, nStrView Desc, Args&&... args) noexcept
			: natException(nestedException, Src, File, Line, Desc, std::forward<Args>(args)...), m_Errno(ErrNo)
		{
			m_Description.Append(natUtil::FormatString(u8" (Errno = {0}, Message = {1})"_nv, m_Errno, GetErrMsg()));
		}

		~natErrException();

		NatErr GetErrNo() const noexcept;
		nStrView GetErrMsg() const noexcept;

	private:
		NatErr m_Errno;

		static NATINLINE nStrView GetErrDescription(NatErr Errno)
		{
			switch (Errno)
			{
			case NatErr_Interrupted:
				return u8"Interrupted"_nv;
			case NatErr_OK:
				return u8"Success"_nv;
			case NatErr_Unknown:
				return u8"Unknown error"_nv;
			case NatErr_IllegalState:
				return u8"Illegal state"_nv;
			case NatErr_InvalidArg:
				return u8"Invalid argument"_nv;
			case NatErr_InternalErr:
				return u8"Internal error"_nv;
			case NatErr_OutOfRange:
				return u8"Out of range"_nv;
			case NatErr_NotImpl:
				return u8"Not implemented"_nv;
			case NatErr_NotSupport:
				return u8"Not supported"_nv;
			case NatErr_Duplicated:
				return u8"Duplicated"_nv;
			case NatErr_NotFound:
				return u8"Not found"_nv;
			default:
				assert(!"Invalid Errno.");
				return u8"No description"_nv;
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
		: BaseException(std::move(nestedException), Src, File, Line, DefaultDescription)\
	{\
	}\
	template <typename... Args>\
	ExceptionClass(nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args)\
		: BaseException(Src, File, Line, Desc, std::forward<Args>(args)...)\
	{\
	}\
	template <typename... Args>\
	ExceptionClass(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args)\
		: BaseException(std::move(nestedException), Src, File, Line, Desc, std::forward<Args>(args)...)\
	{\
	}\
}

	DeclareException(MemoryAllocFail, natException, u8"Failed to allocate memory."_nv);
	DeclareException(InvalidData, natException, u8"Data is invalid."_nv);

	class OutOfRange
		: public natErrException
	{
	public:
		typedef natErrException BaseException;

		OutOfRange(nStrView Src, nStrView File, nuInt Line);
		OutOfRange(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line);

		template <typename... Args>
		OutOfRange(nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args)
			: BaseException(Src, File, Line, NatErr_OutOfRange, Desc, std::forward<Args>(args)...)
		{
		}
		template <typename... Args>
		OutOfRange(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args)
			: BaseException(nestedException, Src, File, Line, NatErr_OutOfRange, Desc, std::forward<Args>(args)...)
		{
		}
	};

	class NotImplementedException
		: public natErrException
	{
	public:
		typedef natErrException BaseException;

		NotImplementedException(nStrView Src, nStrView File, nuInt Line);
		NotImplementedException(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line);

		template <typename... Args>
		NotImplementedException(nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args)
			: BaseException(Src, File, Line, NatErr_NotImpl, Desc, std::forward<Args>(args)...)
		{
		}
		template <typename... Args>
		NotImplementedException(std::exception_ptr nestedException, nStrView Src, nStrView File, nuInt Line, nStrView Desc, Args&&... args)
			: BaseException(nestedException, Src, File, Line, NatErr_NotImpl, Desc, std::forward<Args>(args)...)
		{
		}
		~NotImplementedException();
	};
}

#define nat_Throw(ExceptionClass, ...) do { throw ExceptionClass{ nStrView{ __FUNCTION__ }, nStrView{ __FILE__ }, static_cast<nuInt>(__LINE__), __VA_ARGS__ }; } while (false)
#define nat_ThrowWithNested(ExceptionClass, ...) do { std::throw_with_nested(ExceptionClass{ std::current_exception(), nStrView{ __FUNCTION__ }, nStrView{ __FILE__ }, static_cast<nuInt>(__LINE__), __VA_ARGS__ }); } while (false)
#define nat_ThrowIfFailed(Expression, ...) do { nResult result; if (NATFAIL(result = (Expression))) nat_Throw(natErrException, static_cast<NatErr>(result), __VA_ARGS__); } while (false)

#include "natStringUtil.h"
