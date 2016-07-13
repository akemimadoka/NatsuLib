#include <iostream>

#define TRACEREFOBJ 1

#include <natUtil.h>
#include <natMisc.h>
#include <natConcepts.h>
#include <natLog.h>
#include <natMultiThread.h>
#include <natLinq.h>

using namespace NatsuLib;

struct Incrementable
{
	template <typename T>
	auto requires_(T&& x) -> decltype(++x);
};

template <typename T>
REQUIRES(void, std::conjunction<Models<Incrementable(T)>>::value) increment(T& x)
{
	++x;
}

int main()
{
	std::locale defaultLocale("", LC_CTYPE);
	std::locale::global(defaultLocale);
	std::wcout.imbue(defaultLocale);
	std::wclog.imbue(defaultLocale);
	std::wcerr.imbue(defaultLocale);

	natEventBus eventBus;
	natLog logger(eventBus);
	logger.RegisterLogUpdateEventFunc([] (natEventBase& event)
	{
		decltype(auto) eventLogUpdated = static_cast<natLog::EventLogUpdated&>(event);
		time_t time = std::chrono::system_clock::to_time_t(eventLogUpdated.GetTime());
		tm timeStruct;
		localtime_s(&timeStruct, &time);
		natLog::LogType logType = static_cast<natLog::LogType>(eventLogUpdated.GetLogType());
		nTString logStr = std::move(natUtil::FormatString(_T("[{0}] [{1}] {2}"), std::put_time(&timeStruct, _T("%F %T")), natLog::GetDefaultLogTypeName(logType), eventLogUpdated.GetData()));
		switch (logType)
		{
		case natLog::Msg:
		case natLog::Warn:
			std::wclog << logStr << std::endl;
			break;
		case natLog::Err:
			std::wcerr << logStr << std::endl;
			break;
		default:
			break;
		}
	});

	try
	{
		logger.LogMsg(_T("{1} {0}"), _T("²âÊÔÖÐÎÄ"), 123);

		int t = 5;
		increment(t);
		auto scope = make_scope([&logger](int i)
		{
			logger.LogMsg(_T("%s%d"), _T("end"), i);
		}, t);
		
		{
			std::vector<nTString> strvec;
			natUtil::split(_T("test 2333"), _T(" 2"), [&strvec](ncTStr str, size_t len)
			{
				strvec.emplace_back(str, len);
			});
			for (auto&& item : strvec)
			{
				logger.LogMsg(_T("%s"), item);
			}
		}

		{
			int arr[] = { 1, 2, 3, 4, 5 };
			for (auto&& item : from(make_range(arr)).select([](int i){ return i + 1; }).where([](int i){ return i > 3; }))
			{
				logger.LogMsg(_T("%d"), item);
			}
			logger.LogMsg(_T("%d"), from_values(2, 4, 6, 3, 2).where([](int i) {return i >= 4; }).count());
		}

		{
			natThreadPool pool(2, 4);
			auto ret = pool.QueueWork([](void* Param)
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(10ms);
				static_cast<natLog*>(Param)->LogMsg(_T("test"));
				return 0u;
			}, &logger);
			auto&& result = ret.get();
			logger.LogMsg(_T("Work started at thread index {0}, id {1}."), result.GetWorkThreadIndex(), pool.GetThreadId(result.GetWorkThreadIndex()));
			logger.LogMsg(_T("Work finished with result {0}."), result.GetResult().get());
			pool.WaitAllJobsFinish();
		}
	}
	catch (natWinException& e)
	{
		logger.LogErr(_T("Exception caught from {0}, file \"{1}\" line {2},\nDescription: {3}\nErrno: {4}, Msg: {5}"), e.GetSource(), e.GetFile(), e.GetLine(), e.GetDesc(), e.GetErrNo(), e.GetErrMsg());
	}
	catch (natException& e)
	{
		logger.LogErr(_T("Exception caught from {0}, file \"{1}\" line {2},\nDescription: {3}"), e.GetSource(), e.GetFile(), e.GetLine(), e.GetDesc());
	}

	system("pause");
}
