#include "stdafx.h"
#include "natStackWalker.h"

#ifdef EnableStackWalker

#include "natMisc.h"
#include "natException.h"

using namespace NatsuLib;

#ifdef _WIN32
std::atomic<nBool> natStackWalker::s_Initialized { false };

namespace
{
	natScope<std::function<void()>> StackWalkerUninitializer{ []
	{
		if (natStackWalker::HasInitialized())
		{
			SymCleanup(GetCurrentProcess());
		}
	} };
}

natStackWalker::natStackWalker(nStrView userSearchPath)
{
	if (!s_Initialized.load(std::memory_order_acquire))
	{
		SymSetOptions(SYMOPT_LOAD_LINES);
		if (!SymInitialize(GetCurrentProcess(),
#ifdef UNICODE
			userSearchPath.empty() ? AnsiString{userSearchPath}.data() : nullptr
#else
			userSearchPath.data()
#endif
			, TRUE))
		{
			nat_Throw(natWinException, "SymInitialize failed."_nv);
		}

		s_Initialized.store(true, std::memory_order_release);
	}
}

natStackWalker::~natStackWalker()
{
}

void natStackWalker::CaptureStack(size_t skipFrames, nStrView unknownSymbolName, nStrView unknownFileName) noexcept
{
#if WINVER <= _WIN32_WINNT_WS03
	if (skipFrames > 63)
	{
		nat_Throw(natException, "skipFrames is too large for Windows Server 2003 / Windows XP, see https://msdn.microsoft.com/en-us/library/windows/desktop/bb204633(v=vs.85).aspx ."_nv);
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
	} symbol{};
	symbol.SymbolInfo.SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol.SymbolInfo.MaxNameLen = MAX_SYM_NAME;
	IMAGEHLP_LINE64 line{};
	line.SizeOfStruct = sizeof line;
	DWORD displacement;

	m_StackSymbols.reserve(frames);
	for (size_t i = 0; i < frames; ++i)
	{
		Symbol currentSymbol{ pStack[i] };
		if (!SymFromAddr(hProcess, reinterpret_cast<DWORD64>(pStack[i]), nullptr, &symbol.SymbolInfo))
		{
			currentSymbol.SymbolName = unknownSymbolName.empty() ? unknownSymbolName : "Unknown"_nv;
			currentSymbol.SymbolAddress = 0;
		}
		else
		{
			currentSymbol.SymbolName = AnsiStringView{ symbol.SymbolInfo.Name, symbol.SymbolInfo.NameLen };
			currentSymbol.SymbolAddress = symbol.SymbolInfo.Address;
		}

		if (!SymGetLineFromAddr64(hProcess, reinterpret_cast<DWORD64>(pStack[i]), &displacement, &line))
		{
			currentSymbol.SourceFileName = unknownFileName.empty() ? unknownFileName : "Unknown"_nv;
			currentSymbol.SourceFileAddress = 0;
			currentSymbol.SourceFileLine = 0;
		}
		else
		{
			currentSymbol.SourceFileName = AnsiStringView{ line.FileName };
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
		nat_Throw(natErrException, NatErr_InvalidArg, "Invalid frame."_nv);
	}

	return m_StackSymbols[frame];
}

nBool natStackWalker::HasInitialized() noexcept
{
	return s_Initialized.load(std::memory_order_acquire);
}
#else

#include <cstdlib>

natStackWalker::natStackWalker()
{
}

natStackWalker::~natStackWalker()
{
}

void natStackWalker::CaptureStack(size_t captureFrames, nStrView unknownSymbolInfo) noexcept
{
	std::vector<AddressType> addresses(captureFrames);
	const auto size = static_cast<size_t>(backtrace(addresses.data(), static_cast<int>(captureFrames)));
	addresses.resize(size);
	const auto symbolInfo = backtrace_symbols(addresses.data(), static_cast<int>(size));
	const auto scope = make_scope([symbolInfo]
	{
		std::free(symbolInfo);
	});

	for (size_t i = 0; i < size; ++i)
	{
		const auto symInfo = symbolInfo[i];
		Symbol currentSymbol { addresses[i], symInfo ? nStrView{ symInfo } : unknownSymbolInfo.empty() ? "Unknown"_nv : unknownSymbolInfo };
		m_StackSymbols.emplace_back(std::move(currentSymbol));
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

const natStackWalker::Symbol &natStackWalker::GetSymbol(size_t frame) const
{
	if (frame >= m_StackSymbols.size())
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "Invalid frame."_nv);
	}

	return m_StackSymbols[frame];
}

#endif

#endif
