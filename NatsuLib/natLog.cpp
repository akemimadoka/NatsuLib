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
	EventLogUpdated event(type, std::chrono::system_clock::now(), log);
	m_EventBus.Post(event);
}

ncTStr natLog::GetDefaultLogTypeName(LogType logtype)
{
	switch (logtype)
	{
	case Msg:
		return "Message"_nv;
	case Err:
		return "Error"_nv;
	case Warn:
		return "Warning"_nv;
	default:
		return "Unknown"_nv;
	}
}

void natLog::RegisterLogUpdateEventFunc(natEventBus::EventListenerDelegate func)
{
	m_EventBus.RegisterEventListener<EventLogUpdated>(func);
}
