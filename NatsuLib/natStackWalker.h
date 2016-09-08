#pragma once
#include "natConfig.h"

#ifdef EnableStackWalker
#ifdef _WIN32
#include <Windows.h>
#pragma warning (push)
#pragma warning (disable : 4091)
#include <DbgHelp.h>
#pragma warning (pop)
#pragma comment (lib, "DbgHelp.lib")
#else
#endif
#include <limits>
#include <vector>
#include <atomic>

#pragma push_macro("max")
#undef max

namespace NatsuLib
{
	class natStackWalker final
	{
#ifdef _WIN32
		enum
		{
			CaptureFrames =
#if WINVER <= _WIN32_WINNT_WS03
			63
#else
			std::numeric_limits<USHORT>::max()
#endif
		};

		typedef PVOID AddressType;
		typedef decltype(SYMBOL_INFO::Address) SymbolAddressType;
		typedef decltype(IMAGEHLP_LINE64::Address) SourceFileAddressType;
		typedef decltype(IMAGEHLP_LINE64::LineNumber) SourceFileLineNumberType;

		struct Symbol
		{
			AddressType OriginalAddress;
			nTString SymbolName;
			SymbolAddressType SymbolAddress;
			nTString SourceFileName;
			SourceFileAddressType SourceFileAddress;
			SourceFileLineNumberType SourceFileLine;
		};
#else
#endif
	public:
#ifdef _WIN32
		explicit natStackWalker(ncTStr userSearchPath = nullptr);
#else
#endif
		~natStackWalker();

		void CaptureStack(size_t skipFrames = 0, ncTStr unknownSymbolName = nullptr, ncTStr unknownFileName = nullptr) noexcept;
		void Clear() noexcept;
		size_t GetFrameCount() const noexcept;
		Symbol const& GetSymbol(size_t frame) const;

		static nBool HasInitialized() noexcept;

	private:
#ifdef _WIN32
		static std::atomic_bool s_Initialized;
		std::vector<Symbol> m_StackSymbols;
#else
#endif
	};
}

#pragma pop_macro("max")
#endif
