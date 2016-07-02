#include <iostream>

#define TRACEREFOBJ 1

#include <natUtil.h>
#include <natMisc.h>
#include <natConcepts.h>
#include <natLog.h>
#include <natNamedPipe.h>

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
		std::vector<nTString> strvec;
		natUtil::split(_T("test 2333"), _T(" 2"), [&strvec](ncTStr str, size_t len)
		{
			strvec.emplace_back(str, len);
		});
		for (auto&& item : strvec)
		{
			logger.LogMsg(_T("%s"), item);
		}
		
		int arr[] = { 1, 2, 3, 4, 5 };
		for (auto&& item : make_range(arr).pop_front().pop_back(2))
		{
			logger.LogMsg(_T("%d"), item);
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
