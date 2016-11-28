#include <iostream>

#include <natUtil.h>
#include <natMisc.h>
#include <natConcepts.h>
#include <natLog.h>
#include <natMultiThread.h>
#include <natLinq.h>
#include <natStackWalker.h>
#include <natString.h>
#include <natStream.h>
#include <natStreamHelper.h>

using namespace NatsuLib;

struct Incrementable
{
	template <typename T>
	auto requires_(T&& x) -> decltype(++x);

	int foo;
};

template <typename T>
REQUIRES(void, std::conjunction<Models<Incrementable(T)>>::value) increment(T& x)
{
	++x;
}

HasMemberTrait(foo);

int main()
{
	natEventBus eventBus;
	natLog logger(eventBus);
	natConsole console;
	logger.UseDefaultAction(console);

	try
	{
		logger.LogMsg("{1} {0}"_nv, "≤‚ ‘÷–Œƒ"_av, 123);
		
		constexpr auto test = HasMemberNamedfoo<Incrementable>::value;

		int t = 5;
		increment(t);
		auto scope = make_scope([&logger](int i)
		{
			logger.LogMsg("%s%d"_nv, "end"_nv, i);
		}, t);
		
		/*{
			std::vector<nTString> strvec;
			natUtil::split("test 2333"_nv, " 2"_nv, [&strvec](ncTStr str, size_t len)
			{
				strvec.emplace_back(str, len);
			});
			for (auto&& item : strvec)
			{
				logger.LogMsg("%s"_nv, item);
			}
		}*/

		{
			int arr[] = { 1, 2, 3, 4, 5 };
			for (auto&& item : from(arr).select([](int i){ return i + 1; }).where([](int i){ return i > 3; }))
			{
				logger.LogMsg("%d"_nv, item);
			}
			logger.LogMsg("%d"_nv, from_values(2, 4, 6, 3, 2).where([](int i) { return i >= 4; }).count());
		}

		{
			natThreadPool pool{ 2, 4 };
			auto ret = pool.QueueWork([](void* Param)
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(10ms);
				static_cast<natLog*>(Param)->LogMsg("test"_nv);
				return 0u;
			}, &logger);
			auto&& result = ret.get();
			logger.LogMsg("Work started at thread index {0}, id {1}."_nv, result.GetWorkThreadIndex(), pool.GetThreadId(result.GetWorkThreadIndex()));
			logger.LogMsg("Work finished with result {0}."_nv, result.GetResult().get());
			pool.WaitAllJobsFinish();
		}
#ifdef EnableStackWalker
		{
			natStackWalker stackWalker;
			stackWalker.CaptureStack();
			logger.LogMsg("Call stack:"_nv);
			for (size_t i = 0; i < stackWalker.GetFrameCount(); ++i)
			{
				auto&& symbol = stackWalker.GetSymbol(i);
				logger.LogMsg("{3}: (0x%p) {4} at address 0x%p (file {5}:{6} at address 0x%p)"_nv, symbol.OriginalAddress, reinterpret_cast<const void*>(symbol.SymbolAddress), reinterpret_cast<const void*>(symbol.SourceFileAddress), i, symbol.SymbolName, symbol.SourceFileName, symbol.SourceFileLine);
			}
		}
#endif
		{
			natCriticalSection cs;
			natRefScopeGuard<natCriticalSection> sg(cs);
		}
	}
#ifdef _WIN32
	catch (natWinException& e)
	{
		logger.LogErr("Exception caught from {0}, file \"{1}\" line {2},\nDescription: {3}\nErrno: {4}, Msg: {5}"_nv, e.GetSource(), e.GetFile(), e.GetLine(), e.GetDesc(), e.GetErrNo(), e.GetErrMsg());
#ifdef EnableExceptionStackTrace
		logger.LogErr("Call stack:"_nv);
		for (size_t i = 0; i < e.GetStackWalker().GetFrameCount(); ++i)
		{
			auto&& symbol = e.GetStackWalker().GetSymbol(i);
			logger.LogMsg("{3}: (0x%p) {4} at address 0x%p (file {5}:{6} at address 0x%p)"_nv, symbol.OriginalAddress, reinterpret_cast<const void*>(symbol.SymbolAddress), reinterpret_cast<const void*>(symbol.SourceFileAddress), i, symbol.SymbolName, symbol.SourceFileName, symbol.SourceFileLine);
		}
#endif
	}
#endif
	catch (natErrException& e)
	{
		logger.LogErr("Exception caught from {0}, file \"{1}\" line {2},\nDescription: {3}\nErrno: {4}, Msg: {5}"_nv, e.GetSource(), e.GetFile(), e.GetLine(), e.GetDesc(), e.GetErrNo(), e.GetErrMsg());
#ifdef EnableExceptionStackTrace
		logger.LogErr("Call stack:"_nv);
		for (size_t i = 0; i < e.GetStackWalker().GetFrameCount(); ++i)
		{
			auto&& symbol = e.GetStackWalker().GetSymbol(i);
			logger.LogMsg("{3}: (0x%p) {4} at address 0x%p (file {5}:{6} at address 0x%p)"_nv, symbol.OriginalAddress, reinterpret_cast<const void*>(symbol.SymbolAddress), reinterpret_cast<const void*>(symbol.SourceFileAddress), i, symbol.SymbolName, symbol.SourceFileName, symbol.SourceFileLine);
		}
#endif
	}
	catch (natException& e)
	{
		logger.LogErr("Exception caught from {0}, file \"{1}\" line {2},\nDescription: {3}"_nv, e.GetSource(), e.GetFile(), e.GetLine(), e.GetDesc());
#ifdef EnableExceptionStackTrace
		logger.LogErr("Call stack:"_nv);
		for (size_t i = 0; i < e.GetStackWalker().GetFrameCount(); ++i)
		{
			auto&& symbol = e.GetStackWalker().GetSymbol(i);
			logger.LogMsg("{3}: (0x%p) {4} at address 0x%p (file {5}:{6} at address 0x%p)"_nv, symbol.OriginalAddress, reinterpret_cast<const void*>(symbol.SymbolAddress), reinterpret_cast<const void*>(symbol.SourceFileAddress), i, symbol.SymbolName, symbol.SourceFileName, symbol.SourceFileLine);
		}
#endif
	}

#ifdef _WIN32
	system("pause");
#else
	getchar();
#endif
}
