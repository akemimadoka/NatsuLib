#include "stdafx.h"
#include "natStackWalker.h"
#include "natStringUtil.h"

using namespace NatsuLib;

std::atomic_bool natStackWalker::s_Initialized{ false };

#ifdef WIN32
natScope<std::function<void()>> natStackWalker::s_Uninitializer{[]
{
	if (HasInitialized())
	{
		SymCleanup(GetCurrentProcess());
	}
}};

natStackWalker::natStackWalker(ncTStr userSearchPath)
{
	if (!s_Initialized)
	{
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

void natStackWalker::CaptureStack(size_t skipFrames)
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
	auto frames = CaptureStackBackTrace(skipFrames, CaptureFrames - skipFrames, pStack, nullptr);
	Symbol symbol{ 0 };
	symbol.SymbolInfo.SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol.SymbolInfo.MaxNameLen = MAX_SYM_NAME;

	m_StackSymbols.reserve(frames);
	for (size_t i = 0; i < frames; ++i)
	{
		if (!SymFromAddr(hProcess, reinterpret_cast<DWORD64>(pStack[i]), nullptr, &symbol.SymbolInfo))
		{
			nat_Throw(natWinException, _T("SymFromAddr failed."));
		}
		m_StackSymbols.emplace_back(symbol);
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

nTString natStackWalker::GetSymbolName(size_t frame) const
{
	if (frame >= m_StackSymbols.size())
	{
		nat_Throw(natException, _T("Invalid frame."));
	}

	auto&& symbol = m_StackSymbols[frame];

#ifdef UNICODE
	return natUtil::C2Wstr(std::string{ symbol.SymbolInfo.Name, symbol.SymbolInfo.NameLen });
#else
	return std::string{ symbol.SymbolInfo.Name, symbol.SymbolInfo.NameLen };
#endif
}

natStackWalker::AddressType natStackWalker::GetAddress(size_t frame) const
{
	if (frame >= m_StackSymbols.size())
	{
		nat_Throw(natException, _T("Invalid frame."));
	}

	return m_StackSymbols[frame].SymbolInfo.Address;
}

nBool natStackWalker::HasInitialized()
{
	return s_Initialized;
}

#else
#endif
