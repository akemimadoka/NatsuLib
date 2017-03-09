#include "stdafx.h"
#include "natConsole.h"

using namespace NatsuLib;

namespace NatsuLib
{
	namespace detail_
	{
#ifdef _WIN32
		constexpr WORD ForegroundMask = 0x000F;
		constexpr WORD BackgroundMask = 0x00F0;
		constexpr WORD AllColorMask = ForegroundMask | BackgroundMask;

		CONSOLE_SCREEN_BUFFER_INFO GetBufferInfo()
		{
			const auto outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
			if (!outputHandle || outputHandle == INVALID_HANDLE_VALUE)
			{
				nat_Throw(natWinException, "GetStdHandle failed."_nv);
			}

			// 懒得检查
			const auto errorHandle = GetStdHandle(STD_ERROR_HANDLE);
			const auto inputHandle = GetStdHandle(STD_INPUT_HANDLE);

			CONSOLE_SCREEN_BUFFER_INFO info;
			if (!GetConsoleScreenBufferInfo(outputHandle, &info) && !GetConsoleScreenBufferInfo(errorHandle, &info) && !GetConsoleScreenBufferInfo(inputHandle, &info))
			{
				nat_Throw(natWinException, "GetConsoleScreenBufferInfo failed."_nv);
			}

			return info;
		}

		WORD ConsoleColorToFlag(natConsole::ConsoleColorTarget target, natConsole::ConsoleColor color)
		{
			auto rawFlag = static_cast<WORD>(color);
			if (target == natConsole::ConsoleColorTarget::Background)
			{
				rawFlag <<= 4;
			}

			assert(rawFlag <= 0xFF);
			return rawFlag;
		}

		std::pair<natConsole::ConsoleColor, natConsole::ConsoleColor> FlagToConsoleColor(DWORD flag)
		{
			return { static_cast<natConsole::ConsoleColor>(flag & ForegroundMask), static_cast<natConsole::ConsoleColor>((flag & BackgroundMask) >> 4) };
		}
#endif
	}
}

natConsole::natConsole()
	: m_StdIn{ natStdStream::StdIn }, m_StdOut{ natStdStream::StdOut }, m_StdErr{ natStdStream::StdErr }, m_StdInReader{ &m_StdIn
#ifndef _WIN32
		, 1
#endif
	}, m_StdOutWriter{ &m_StdOut }, m_StdErrWriter{ &m_StdErr }
{
}

nString natConsole::GetTitle() const
{
#ifdef _WIN32
	TCHAR titleBuffer[MAX_PATH];
	const auto result = GetConsoleTitle(titleBuffer, MAX_PATH);
	if (!result)
	{
		nat_Throw(natWinException, "GetConsoleTitle failed."_nv);
	}
	
	return
#ifdef _UNICODE
		WideStringView{ titleBuffer, result };
#else
		AnsiStringView{ titleBuffer, result };
#endif

#else
	nat_Throw(natErrException, NatErr_NotSupport, "This platform does not support this operation."_nv);
#endif
}

void natConsole::SetTitle(nStrView title)
{
#ifdef _WIN32
	if (!SetConsoleTitle(
#ifdef _UNICODE
		WideString{ title }.data()
#else
		AnsiString{ title }.data()
#endif
		))
	{
		nat_Throw(natWinException, "SetConsoleTitle failed."_nv);
	}
#else
	nat_Throw(NotImplementedException);
#endif
}

natConsole::ConsoleColor natConsole::GetColor(ConsoleColorTarget target) const
{
#ifdef _WIN32
	const auto bufferInfo = detail_::GetBufferInfo();

	ConsoleColor foregroundColor, backgroundColor;
	std::tie(foregroundColor, backgroundColor) = detail_::FlagToConsoleColor(bufferInfo.wAttributes);

	switch (target)
	{
	case ConsoleColorTarget::Foreground:
		return foregroundColor;
	case ConsoleColorTarget::Background:
	default:
		assert(target == ConsoleColorTarget::Background);
		return backgroundColor;
	}
#else
	nat_Throw(NotImplementedException);
#endif
}

void natConsole::SetColor(ConsoleColorTarget target, ConsoleColor color)
{
#ifdef _WIN32
	const auto bufferInfo = detail_::GetBufferInfo();

	if (!m_DefaultColor)
	{
		m_DefaultColor.emplace(static_cast<WORD>(bufferInfo.wAttributes & detail_::AllColorMask));
	}

	auto flag = detail_::ConsoleColorToFlag(target, color);

	switch (target)
	{
	case ConsoleColorTarget::Foreground:
		flag |= bufferInfo.wAttributes & ~detail_::ForegroundMask;
		break;
	case ConsoleColorTarget::Background:
	default:
		assert(target == ConsoleColorTarget::Background);
		flag |= bufferInfo.wAttributes & ~detail_::BackgroundMask;
		break;
	}

	if (!SetConsoleTextAttribute(m_StdOut.GetNativeHandle(), flag))
	{
		nat_Throw(natWinException, "SetConsoleTextAttribute failed."_nv);
	}
#else
	nat_Throw(NotImplementedException);
#endif
}

void natConsole::ResetColor()
{
#ifdef _WIN32
	if (!m_DefaultColor)
	{
		// 默认颜色未被缓存或者未被修改过
		return;
	}

	const auto bufferInfo = detail_::GetBufferInfo();

	if (!SetConsoleTextAttribute(m_StdOut.GetNativeHandle(), bufferInfo.wAttributes & ~detail_::AllColorMask | m_DefaultColor.value()))
	{
		nat_Throw(natWinException, "SetConsoleTextAttribute failed."_nv);
	}
#else
	nat_Throw(NotImplementedException);
#endif
}

void natConsole::Write(StringView<Encoding> const& str)
{
	m_StdOutWriter.Write(str);
}

void natConsole::WriteLine(StringView<Encoding> const& str)
{
	m_StdOutWriter.WriteLine(str);
}

void natConsole::WriteErr(StringView<Encoding> const& str)
{
	m_StdErrWriter.Write(str);
}

void natConsole::WriteLineErr(StringView<Encoding> const& str)
{
	m_StdErrWriter.WriteLine(str);
}
