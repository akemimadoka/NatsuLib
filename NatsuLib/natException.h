////////////////////////////////////////////////////////////////////////////////
///	@file	natException.h
///	@brief	异常相关头文件
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natType.h"
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
		nTString FormatString(ncTStr lpStr, Args&&... args);
		std::wstring C2Wstr(std::string const& str);
		std::string W2Cstr(std::wstring const& str);
	}

	namespace detail_
	{
		struct natExceptionStorage
		{
			natExceptionStorage(std::exception_ptr nestedException, std::chrono::system_clock::time_point const& time, nTString const& file, nuInt line, nTString const& src, nTString const& desc)
				: m_NestedException(nestedException), m_Time(time), m_File(file), m_Line(line), m_Source(src), m_Description(desc)
#ifdef UNICODE
				, m_MBDescription(natUtil::W2Cstr(m_Description))
#endif
			{
			}

			std::exception_ptr m_NestedException;
			std::chrono::system_clock::time_point m_Time;
			nTString m_File;
			nuInt m_Line;
			nTString m_Source;
			nTString m_Description;
#ifdef UNICODE
			std::string m_MBDescription;
#endif
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
		natException(std::exception_ptr nestedException, ncTStr Src, ncTStr File, nuInt Line, ncTStr Desc, Args&&... args) noexcept
#ifdef UNICODE
			: Storage{ nestedException, std::chrono::system_clock::now(), File, Line, Src, natUtil::FormatString(Desc, std::forward<Args>(args)...) }
#else
			: Storage{ nestedException, std::chrono::system_clock::now(), File, Line, Src, natUtil::FormatString(Desc, std::forward<Args>(args)...) }
#endif
#ifdef EnableExceptionStackTrace
			, m_StackWalker()
#endif
		{
#ifdef EnableExceptionStackTrace
			m_StackWalker.CaptureStack();
#endif
		}

		template <typename... Args>
		natException(ncTStr Src, ncTStr File, nuInt Line, ncTStr Desc, Args&&... args) noexcept
			: natException(std::exception_ptr{}, Src, File, Line, Desc, std::forward<Args>(args)...)
		{
		}

		virtual ~natException() = default;

		std::chrono::system_clock::time_point GetTime() const noexcept
		{
			return m_Time;
		}

		ncTStr GetFile() const noexcept
		{
			return m_File.c_str();
		}

		nuInt GetLine() const noexcept
		{
			return m_Line;
		}

		ncTStr GetSource() const noexcept
		{
			return m_Source.c_str();
		}

		ncTStr GetDesc() const noexcept
		{
			return m_Description.c_str();
		}

		std::exception_ptr GetNestedException() const noexcept
		{
			return m_NestedException;
		}

#ifdef EnableExceptionStackTrace
		natStackWalker const& GetStackWalker() const noexcept
		{
			return m_StackWalker;
		}
#endif

		ncStr what() const noexcept override
		{
#ifdef UNICODE
			return m_MBDescription.c_str();
#else
			return m_Description.c_str();
#endif
		}

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
		natWinException(ncTStr Src, ncTStr File, nuInt Line, ncTStr Desc, Args&&... args) noexcept
			: natWinException(Src, File, Line, GetLastError(), Desc, std::forward<Args>(args)...)
		{
		}

		template <typename... Args>
		natWinException(std::exception_ptr nestedException, ncTStr Src, ncTStr File, nuInt Line, ncTStr Desc, Args&&... args) noexcept
			: natWinException(nestedException, Src, File, Line, GetLastError(), Desc, std::forward<Args>(args)...)
		{
		}

		template <typename... Args>
		natWinException(ncTStr Src, ncTStr File, nuInt Line, DWORD LastErr, ncTStr Desc, Args&&... args) noexcept
			: natException(Src, File, Line, Desc, std::forward<Args>(args)...), m_LastErr(LastErr), m_ErrMsg()
		{
			m_Description = natUtil::FormatString((m_Description + _T(" (LastErr = {0})")).c_str(), m_LastErr);
		}

		template <typename... Args>
		natWinException(std::exception_ptr nestedException, ncTStr Src, ncTStr File, nuInt Line, DWORD LastErr, ncTStr Desc, Args&&... args) noexcept
			: natException(nestedException, Src, File, Line, Desc, std::forward<Args>(args)...), m_LastErr(LastErr), m_ErrMsg()
		{
			m_Description = natUtil::FormatString((m_Description + _T(" (LastErr = {0})")).c_str(), m_LastErr);
		}

		DWORD GetErrNo() const noexcept
		{
			return m_LastErr;
		}

		ncTStr GetErrMsg() const noexcept
		{
			if (m_ErrMsg.empty())
			{
				LPVOID pBuf = nullptr;
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, m_LastErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<nTStr>(&pBuf), 0, nullptr);
				if (pBuf)
				{
					m_ErrMsg = static_cast<ncTStr>(pBuf);
					LocalFree(pBuf);
				}
			}

			return m_ErrMsg.c_str();
		}

	private:
		DWORD m_LastErr;
		mutable nTString m_ErrMsg;
	};
#endif

	class natErrException
		: public natException
	{
	public:
		template <typename... Args>
		natErrException(ncTStr Src, ncTStr File, nuInt Line, NatErr ErrNo, ncTStr Desc, Args&&... args) noexcept
			: natException(Src, File, Line, Desc, std::forward<Args>(args)...), m_Errno(ErrNo)
		{
			m_Description = natUtil::FormatString((m_Description + _T(" (Errno = {0})")).c_str(), m_Errno);
		}

		template <typename... Args>
		natErrException(std::exception_ptr nestedException, ncTStr Src, ncTStr File, nuInt Line, NatErr ErrNo, ncTStr Desc, Args&&... args) noexcept
			: natException(nestedException, Src, File, Line, Desc, std::forward<Args>(args)...), m_Errno(ErrNo)
		{
			m_Description = natUtil::FormatString((m_Description + _T(" (Errno = {0})")).c_str(), m_Errno);
		}

		NatErr GetErrNo() const noexcept
		{
			return m_Errno;
		}

		ncTStr GetErrMsg() const noexcept
		{
			return GetErrDescription(m_Errno);
		}

	private:
		NatErr m_Errno;

		static ncTStr GetErrDescription(NatErr Errno)
		{
			switch (Errno)
			{
			case NatErr_Interrupted:
				return _T("Interrupted");
			case NatErr_OK:
				return _T("Success");
			case NatErr_Unknown:
				return _T("Unknown error");
			case NatErr_IllegalState:
				return _T("Illegal state");
			case NatErr_InvalidArg:
				return _T("Invalid argument");
			case NatErr_InternalErr:
				return _T("Internal error");
			case NatErr_OutOfRange:
				return _T("Out of range");
			case NatErr_NotImpl:
				return _T("Not implemented");
			case NatErr_NotSupport:
				return _T("Not supported");
			case NatErr_Duplicated:
				return _T("Duplicated");
			case NatErr_NotFound:
				return _T("Not found");
			default:
				return _T("No description");
			}
		}
	};

#define DeclareException(ExceptionClass, ExtendException, DefaultDescription) \
class ExceptionClass : public ExtendException\
{\
public:\
	typedef ExtendException BaseException;\
	ExceptionClass(ncTStr Src, ncTStr File, nuInt Line)\
		: BaseException(Src, File, Line, DefaultDescription)\
	{\
	}\
	ExceptionClass(std::exception_ptr nestedException, ncTStr Src, ncTStr File, nuInt Line)\
		: BaseException(nestedException, Src, File, Line, DefaultDescription)\
	{\
	}\
	template <typename... Args>\
	ExceptionClass(ncTStr Src, ncTStr File, nuInt Line, ncTStr Desc, Args&&... args)\
		: BaseException(Src, File, Line, Desc, std::forward<Args>(args)...)\
	{\
	}\
	template <typename... Args>\
	ExceptionClass(std::exception_ptr nestedException, ncTStr Src, ncTStr File, nuInt Line, ncTStr Desc, Args&&... args)\
		: BaseException(nestedException, Src, File, Line, Desc, std::forward<Args>(args)...)\
	{\
	}\
}

	DeclareException(OutOfRange, natException, _T("Out of range."));
	DeclareException(MemoryAllocFail, natException, _T("Failed to allocate memory."));
}

#define nat_Throw(ExceptionClass, ...) do { throw ExceptionClass(_T(__FUNCTION__), _T(__FILE__), __LINE__, __VA_ARGS__); } while (false)
#define nat_ThrowWithNested(ExceptionClass, ...) do { throw ExceptionClass(std::current_exception(), _T(__FUNCTION__), _T(__FILE__), __LINE__, __VA_ARGS__); } while (false)
#define nat_ThrowIfFailed(Expression, ...) do { nResult result; if (NATFAIL(result = (Expression))) nat_Throw(natErrException, static_cast<NatErr>(result), __VA_ARGS__); } while (false)

#include "natStringUtil.h"
