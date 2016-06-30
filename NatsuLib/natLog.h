////////////////////////////////////////////////////////////////////////////////
///	@file	natLog.h
///	@brief	日志相关
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <chrono>

#include "natEvent.h"
#include "natType.h"

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	日志类实现
	///	@note	非线程安全
	///	@see	n2dGlobal::Logfile
	////////////////////////////////////////////////////////////////////////////////
	class natLog final
	{
	public:
		class EventLogUpdated final
			: public natEventBase
		{
		public:
			explicit EventLogUpdated(nuInt logType, std::chrono::system_clock::time_point const& time, ncTStr data)
				: m_LogType(logType), m_Time(time), m_Data(data)
			{
			}

			bool CanCancel() const noexcept override
			{
				return false;
			}

			nuInt GetLogType() const noexcept
			{
				return m_LogType;
			}

			std::chrono::system_clock::time_point const& GetTime() const noexcept
			{
				return m_Time;
			}

			ncTStr GetData() const noexcept
			{
				return m_Data;
			}

		private:
			nuInt m_LogType;
			std::chrono::system_clock::time_point m_Time;
			ncTStr m_Data;
		};

		///	@brief	预置日志类型
		enum LogType
		{
			Msg,	///< @brief	消息
			Err,	///< @brief	错误
			Warn	///< @brief	警告
		};

		explicit natLog(natEventBus& eventBus);
		~natLog();

		///	@brief	记录信息
		template <typename ...Arg>
		void LogMsg(ncTStr content, Arg &&... arg)
		{
			Log(Msg, content, std::forward<Arg>(arg)...);
		}

		///	@brief	记录错误
		template <typename ... Arg>
		void LogErr(ncTStr content, Arg &&... arg)
		{
			Log(Err, content, std::forward<Arg>(arg)...);
		}

		///	@brief	记录警告
		template <typename ... Arg>
		void LogWarn(ncTStr content, Arg &&... arg)
		{
			Log(Warn, content, std::forward<Arg>(arg)...);
		}

		///	@brief	记录
		template <typename ... Arg>
		void Log(nuInt type, ncTStr content, Arg &&... arg)
		{
			UpdateLastLog(type, std::move(natUtil::FormatString(content, std::forward<Arg>(arg)...)));
		}

		///	@brief	获得最新日志
		ncTStr GetLastLog() const;

		///	@brief	注册日志更新事件处理函数
		void RegisterLogUpdateEventFunc(natEventBus::EventListenerFunc func);

		static ncTStr GetDefaultLogTypeName(LogType logtype);

	private:
		void UpdateLastLog(nuInt type, nTString&& log);
		nTString m_LastLog;
		natEventBus& m_EventBus;
	};
}
