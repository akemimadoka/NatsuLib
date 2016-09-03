#pragma once
#ifdef WIN32
#include <Windows.h>
#pragma warning (push)
#pragma warning (disable : 4091)
#include <DbgHelp.h>
#pragma warning (pop)
#pragma comment (lib, "DbgHelp.lib")
#else
#endif
#include <limits>
#include "natType.h"
#include "natMisc.h"
#include <vector>
#include <atomic>

#pragma push_macro("max")
#undef max

namespace NatsuLib
{
	class natStackWalker final
	{
#ifdef WIN32
		enum
		{
			CaptureFrames =
#if WINVER <= _WIN32_WINNT_WS03
			63
#else
			std::numeric_limits<USHORT>::max()
#endif
		};

		typedef decltype(SYMBOL_INFO::Address) AddressType;

		union Symbol
		{
			nByte dummy[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
			SYMBOL_INFO SymbolInfo;
		};
#else
#endif
	public:
#ifdef WIN32
		explicit natStackWalker(ncTStr userSearchPath = nullptr);
#else
#endif
		~natStackWalker();

		void CaptureStack(size_t skipFrames = 0);
		void Clear() noexcept;
		size_t GetFrameCount() const noexcept;
		nTString GetSymbolName(size_t frame) const;
		AddressType GetAddress(size_t frame) const;

		static nBool HasInitialized();

	private:
		static std::atomic_bool s_Initialized;
		static natScope<std::function<void()>> s_Uninitializer;
		std::vector<Symbol> m_StackSymbols;
	};
}

#pragma pop_macro("max")
