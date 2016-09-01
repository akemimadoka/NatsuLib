////////////////////////////////////////////////////////////////////////////////
///	@file	natException.h
///	@brief	异常相关头文件
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "natType.h"
#include <chrono>
#ifdef WIN32
#include <Windows.h>
#endif

namespace NatsuLib
{
	namespace natUtil
	{
		template <typename... Args>
		nTString FormatString(ncTStr lpStr, Args&&... args);
		inline std::wstring C2Wstr(std::string const& str);
		inline std::string W2Cstr(std::wstring const& str);
	}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	NatsuLib异常基类
	///	@note	异常由此类派生，请勿使用可能抛出异常的代码
	////////////////////////////////////////////////////////////////////////////////
	class natException
		: public std::exception
	{
	public:
		template <typename... Args>
		constexpr natException(ncTStr Src, ncTStr File, nuInt Line, ncTStr Desc, Args&&... args) noexcept
#ifdef UNICODE
			: exception(natUtil::W2Cstr(natUtil::FormatString(Desc, std::forward<Args>(args)...)).c_str()), m_Time(std::chrono::system_clock::now()), m_File(File), m_Line(Line), m_Source(Src), m_Description(natUtil::C2Wstr(exception::what()))
#else
			: exception(natUtil::FormatString(Desc, std::forward<Args>(args)...).c_str()), m_Time(std::chrono::system_clock::now()), m_File(File), m_Line(Line), m_Source(Src), m_Description(exception::what())
#endif
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

	protected:
		std::chrono::system_clock::time_point m_Time;
		nTString m_File;
		nuInt m_Line;
		nTString m_Source;
		nTString m_Description;
	};

#ifdef WIN32
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	NatsuLib WinAPI调用异常
	///	@note	可以自动附加LastErr信息
	////////////////////////////////////////////////////////////////////////////////
	class natWinException
		: public natException
	{
	public:
		// bug in Visual Studio 2015 Community, see http://stackoverflow.com/questions/32489702/constexpr-with-delegating-constructors
		template <typename... Args>
		natWinException(ncTStr Src, ncTStr File, nuInt Line, ncTStr Desc, Args&&... args) noexcept
			: natWinException(Src, File, Line, GetLastError(), Desc, std::forward<Args>(args)...)
		{
		}

		template <typename... Args>
		constexpr natWinException(ncTStr Src, ncTStr File, nuInt Line, DWORD LastErr, ncTStr Desc, Args&&... args) noexcept
			: natException(Src, File, Line, Desc, std::forward<Args>(args)...), m_LastErr(LastErr), m_ErrMsg()
		{
			m_Description = move(natUtil::FormatString((m_Description + _T(" (LastErr = {0})")).c_str(), m_LastErr));
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
		constexpr natErrException(ncTStr Src, ncTStr File, nuInt Line, NatErr ErrNo, ncTStr Desc, Args&&... args) noexcept
			: natException(Src, File, Line, Desc, std::forward<Args>(args)...), m_Errno(ErrNo)
		{
			m_Description = move(natUtil::FormatString((m_Description + _T(" (Errno = {0})")).c_str(), m_Errno));
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
			default:
				return _T("No description");
			}
		}
	};
}

#define nat_Throw(ExceptionClass, ...) do { throw ExceptionClass(_T(__FUNCTION__), _T(__FILE__), __LINE__, __VA_ARGS__); } while (false)
#define nat_ThrowIfFailed(Expression, ...) do { nResult result; if (NATFAIL(result = (Expression))) nat_Throw(natErrException, static_cast<NatErr>(result), __VA_ARGS__); } while (false)

#include "natStringUtil.h"
