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
