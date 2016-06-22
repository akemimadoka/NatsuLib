////////////////////////////////////////////////////////////////////////////////
///	@file	natLog.h
///	@brief	日志相关
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <fstream>

#include "natEvent.h"
#include "natType.h"
#include "natUtil.h"

////////////////////////////////////////////////////////////////////////////////
///	@brief	日志类实现
///	@note	单例类，使用getInstance方法获得实例，非线程安全\n
///			默认使用全局变量global::Logfile设定日志文件名
///	@see	n2dGlobal::Logfile
////////////////////////////////////////////////////////////////////////////////
class natLog final
{
public:
	class EventLogUpdated final
		: public natEventBase
	{
	public:
		explicit EventLogUpdated(ncTStr data)
			: m_Data(data)
		{
		}

		bool CanCancel() const noexcept override
		{
			return false;
		}

		ncTStr GetData() const noexcept
		{
			return m_Data;
		}

	private:
		ncTStr m_Data;
	};

	///	@brief	日志类型
	enum class LogType
	{
		Msg,	///< @brief	消息
		Err,	///< @brief	错误
		Warn	///< @brief	警告
	};

	///	@brief	获得实例
	static natLog& GetInstance();
	
	///	@brief	记录信息
	template <typename ...Arg>
	void LogMsg(ncTStr Msg, Arg &&... arg)
	{
		Log(LogType::Msg, Msg, std::forward<Arg>(arg)...);
	}

	///	@brief	记录错误
	template <typename ... Arg>
	void LogErr(ncTStr Err, Arg &&... arg)
	{
		Log(LogType::Err, Err, std::forward<Arg>(arg)...);
	}

	///	@brief	记录警告
	template <typename ... Arg>
	void LogWarn(ncTStr Warn, Arg &&... arg)
	{
		Log(LogType::Warn, Warn, std::forward<Arg>(arg)...);
	}

	///	@brief	记录
	template <typename ... Arg>
	void Log(LogType type, ncTStr content, Arg &&... arg)
	{
		UpdateLastLog(std::move(natUtil::FormatString(natUtil::FormatString(_T("[%s] [%s] %s"), natUtil::GetSysTime().c_str(), ParseLogType(type), content), std::forward<Arg>(arg)...)));
	}

	///	@brief	获得日志文件名
	ncTStr GetLogFile() const;

	///	@brief	获得最新日志
	ncTStr GetLastLog() const;

	///	@brief	注册日志更新事件处理函数
	void RegisterLogUpdateEventFunc(natEventBus::EventListenerFunc func);
private:
	explicit natLog(ncTStr const& logfile);
	~natLog();

	void UpdateLastLog(nTString&& log);

	nTString m_LogFile;
	std::basic_ofstream<nTChar> m_fstr;
	nTString m_LastLog;

	static ncTStr ParseLogType(LogType logtype);
};