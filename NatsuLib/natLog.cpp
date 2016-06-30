#include "stdafx.h"
#include "natLog.h"

#pragma warning (disable:4996) 

using namespace NatsuLib;

natLog::natLog(natEventBus& eventBus)
	: m_EventBus(eventBus)
{
	m_EventBus.RegisterEvent<EventLogUpdated>();
}

natLog::~natLog()
{
}

void natLog::UpdateLastLog(nuInt type, nTString&& log)
{
	m_LastLog = move(log);
	EventLogUpdated event(type, std::chrono::system_clock::now(), m_LastLog.c_str());
	m_EventBus.Post(event);
}

ncTStr natLog::GetDefaultLogTypeName(LogType logtype)
{
	switch (logtype)
	{
	case Msg:
		return _T("Message");
	case Err:
		return _T("Error");
	case Warn:
		return _T("Warning");
	default:
		return _T("Unknown");
	}
}

ncTStr natLog::GetLastLog() const
{
	return m_LastLog.c_str();
}

void natLog::RegisterLogUpdateEventFunc(natEventBus::EventListenerFunc func)
{
	m_EventBus.RegisterEventListener<EventLogUpdated>(func);
}
