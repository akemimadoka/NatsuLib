#include "stdafx.h"
#include "natStackWalker.h"

#ifdef EnableStackWalker

#include "natMisc.h"

using namespace NatsuLib;

std::atomic_bool natStackWalker::s_Initialized{ false };

#ifdef WIN32
natScope<std::function<void()>> StackWalkerUninitializer{[]
{
	if (natStackWalker::HasInitialized())
	{
		SymCleanup(GetCurrentProcess());
	}
}};

natStackWalker::natStackWalker(ncTStr userSearchPath)
{
	if (!s_Initialized)
	{
		SymSetOptions(SYMOPT_LOAD_LINES);
		if (!SymInitialize(GetCurrentProcess(),
#ifdef UNICODE
			userSearchPath ? natUtil::W2Cstr(userSearchPath).c_str() : nullptr
#else
			userSearchPath
#endif
			, TRUE))
		{
			nat_Throw(natWinException, _T("SymInitialize failed."));
		}

		s_Initialized = true;
	}
}

natStackWalker::~natStackWalker()
{
}

void natStackWalker::CaptureStack(size_t skipFrames, ncTStr unknownSymbolName, ncTStr unknownFileName) noexcept
{
#if WINVER <= _WIN32_WINNT_WS03
	if (skipFrames > 63)
	{
		nat_Throw(natException, _T("skipFrames is too large for Windows Server 2003 / Windows XP, see https://msdn.microsoft.com/en-us/library/windows/desktop/bb204633(v=vs.85).aspx ."));
	}
#endif

	m_StackSymbols.clear();
	auto hProcess = GetCurrentProcess();
	PVOID pStack[CaptureFrames]{ nullptr };
	auto frames = CaptureStackBackTrace(static_cast<DWORD>(skipFrames), static_cast<DWORD>(CaptureFrames - skipFrames), pStack, nullptr);
	union
	{
		nByte dummy[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
		SYMBOL_INFO SymbolInfo;
	} symbol{ 0 };
	symbol.SymbolInfo.SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol.SymbolInfo.MaxNameLen = MAX_SYM_NAME;
	IMAGEHLP_LINE64 line{ 0 };
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	DWORD displacement;

	m_StackSymbols.reserve(frames);
	for (size_t i = 0; i < frames; ++i)
	{
		Symbol currentSymbol{ pStack[i] };
		if (!SymFromAddr(hProcess, reinterpret_cast<DWORD64>(pStack[i]), nullptr, &symbol.SymbolInfo))
		{
			currentSymbol.SymbolName = unknownSymbolName ? unknownSymbolName : _T("Unknown");
			currentSymbol.SymbolAddress = 0;
		}
		else
		{
#ifdef UNICODE
			currentSymbol.SymbolName = natUtil::C2Wstr(std::string{ symbol.SymbolInfo.Name, symbol.SymbolInfo.NameLen });
#else
			currentSymbol.SymbolName.assign(symbol.SymbolInfo.Name, symbol.SymbolInfo.NameLen);
#endif
			currentSymbol.SymbolAddress = symbol.SymbolInfo.Address;
		}

		if (!SymGetLineFromAddr64(hProcess, reinterpret_cast<DWORD64>(pStack[i]), &displacement, &line))
		{
			currentSymbol.SourceFileName = unknownFileName ? unknownFileName : _T("Unknown");
			currentSymbol.SourceFileAddress = 0;
			currentSymbol.SourceFileLine = 0;
		}
		else
		{
#ifdef UNICODE
			currentSymbol.SourceFileName = natUtil::C2Wstr(line.FileName);
#else
			currentSymbol.SourceFileName = line.FileName;
#endif
			currentSymbol.SourceFileAddress = line.Address;
			currentSymbol.SourceFileLine = line.LineNumber;
		}

		m_StackSymbols.emplace_back(currentSymbol);
	}
}

void natStackWalker::Clear() noexcept
{
	m_StackSymbols.clear();
}

size_t natStackWalker::GetFrameCount() const noexcept
{
	return m_StackSymbols.size();
}

natStackWalker::Symbol const& natStackWalker::GetSymbol(size_t frame) const
{
	if (frame >= m_StackSymbols.size())
	{
		nat_Throw(natException, _T("Invalid frame."));
	}

	return m_StackSymbols[frame];
}

nBool natStackWalker::HasInitialized() noexcept
{
	return s_Initialized;
}

#else
#endif

#endif
