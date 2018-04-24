#pragma once
#include "natConfig.h"
#include "natType.h"
#include "natString.h"

#if EnableStackWalker
#ifdef _WIN32
#include <Windows.h>
#pragma warning (push)
#pragma warning (disable : 4091)
#include <DbgHelp.h>
#pragma warning (pop)
#pragma comment (lib, "DbgHelp.lib")
#else
#include <execinfo.h>
#endif
#include <limits>
#include <vector>
#include <mutex>

#ifdef _MSC_VER
#	pragma push_macro("max")
#endif
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
			nString SymbolName;
			SymbolAddressType SymbolAddress;
			nString SourceFileName;
			SourceFileAddressType SourceFileAddress;
			SourceFileLineNumberType SourceFileLine;
		};
#else
		enum
		{
			CaptureFrames = 255,
		};

		typedef void* AddressType;

		struct Symbol
		{
			AddressType OriginalAddress;
			nString SymbolInfo;
		};
#endif
	public:
#ifdef _WIN32
		explicit natStackWalker(nStrView userSearchPath = nullptr);

		void CaptureStack(std::size_t skipFrames = 0, nStrView unknownSymbolName = nullptr, nStrView unknownFileName = nullptr) noexcept;
#else
		natStackWalker();

		void CaptureStack(std::size_t captureFrames = CaptureFrames, nStrView unknownSymbolInfo = nullptr) noexcept;
#endif
		~natStackWalker();

		void Clear() noexcept;
		std::size_t GetFrameCount() const noexcept;
		Symbol const& GetSymbol(std::size_t frame) const;

	private:
#ifdef _WIN32
		static std::once_flag s_OnceFlag;
		std::vector<Symbol> m_StackSymbols;
#else
		std::vector<Symbol> m_StackSymbols;
#endif
	};
}

#ifdef _MSC_VER
#	pragma pop_macro("max")
#endif

#endif
