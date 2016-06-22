#include "stdafx.h"
#include "natLog.h"

#pragma warning (disable:4996) 

///	@brief	全局变量
namespace n2dGlobal
{
	///	@brief	默认日志文件名
	ncTStr Logfile = _T("Log.log");
}

natLog::natLog(ncTStr const& logfile)
	: m_LogFile(logfile),
	m_fstr(m_LogFile)
{
	natEventBus::GetInstance().RegisterEvent<EventLogUpdated>();
	LogMsg(_T("Log start."));
}

natLog::~natLog()
{
	m_fstr.close();
}

void natLog::UpdateLastLog(nTString&& log)
{
	m_LastLog = move(log);
	m_fstr << m_LastLog << std::endl;
	EventLogUpdated event(m_LastLog.c_str());
	natEventBus::GetInstance().Post(event);
}

ncTStr natLog::ParseLogType(LogType logtype)
{
	switch (logtype)
	{
	case LogType::Msg:
		return _T("Message");
	case LogType::Err:
		return _T("Error");
	case LogType::Warn:
		return _T("Warning");
	default:
		return _T("Unknown");
	}
}

NATNOINLINE natLog& natLog::GetInstance()
{
	static natLog instance(n2dGlobal::Logfile);
	return instance;
}

ncTStr natLog::GetLogFile() const
{
	return m_LogFile.c_str();
}

ncTStr natLog::GetLastLog() const
{
	return m_LastLog.c_str();
}

void natLog::RegisterLogUpdateEventFunc(natEventBus::EventListenerFunc func)
{
	natEventBus::GetInstance().RegisterEventListener<EventLogUpdated>(func);
}
