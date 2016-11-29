#include "stdafx.h"
#include "natConsole.h"

using namespace NatsuLib;

natConsole::natConsole()
	: m_StdIn{ natStdStream::StdIn }, m_StdOut{ natStdStream::StdOut }, m_StdErr{ natStdStream::StdErr }, m_StdInReader{ &m_StdIn
#ifndef _WIN32
		, 1
#endif
	}, m_StdOutWriter{ &m_StdOut }, m_StdErrWriter{ &m_StdErr }
{
#ifdef _WIN32
	if (!SetConsoleCP(CP_UTF8))
	{
		nat_Throw(natWinException, "SetConsoleCP failed."_nv);
	}
	if (!SetConsoleOutputCP(CP_UTF8))
	{
		nat_Throw(natWinException, "SetConsoleOutputCP failed."_nv);
	}
#endif
}

void natConsole::Write(nStrView const& str)
{
	m_StdOutWriter.Write(str);
}

void natConsole::WriteLine(nStrView const& str)
{
	m_StdOutWriter.WriteLine(str);
}

void natConsole::WriteErr(nStrView const& str)
{
	m_StdErrWriter.Write(str);
}

void natConsole::WriteLineErr(nStrView const& str)
{
	m_StdErrWriter.WriteLine(str);
}

nString natConsole::ReadLine()
{
	return m_StdInReader.ReadLine();
}
