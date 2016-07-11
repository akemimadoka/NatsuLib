#include "stdafx.h"
#include "natLog.h"

using namespace NatsuLib;

natLog::natLog(natEventBus& eventBus)
	: m_EventBus(eventBus)
{
	m_EventBus.RegisterEvent<EventLogUpdated>();
}

natLog::~natLog()
{
}

void natLog::UpdateLog(nuInt type, nTString&& log)
{
	EventLogUpdated event(type, std::chrono::system_clock::now(), log.c_str());
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

void natLog::RegisterLogUpdateEventFunc(natEventBus::EventListenerFunc func)
{
	m_EventBus.RegisterEventListener<EventLogUpdated>(func);
}
