////////////////////////////////////////////////////////////////////////////////
///	@file	natLog.h
///	@brief	日志相关
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <chrono>
#include <iostream>

#include "natEvent.h"
#include "natConsole.h"

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	日志类实现
	////////////////////////////////////////////////////////////////////////////////
	class natLog final
	{
	public:
		class EventLogUpdated final
			: public natEventBase
		{
		public:
			EventLogUpdated(nuInt logType, std::chrono::system_clock::time_point const& time, ncTStr data) noexcept
				: m_LogType(logType), m_Time(time), m_Data(data)
			{
			}

			nBool CanCancel() const noexcept override
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
		template <typename... Arg>
		void LogMsg(ncTStr content, Arg &&... arg)
		{
			Log(Msg, content, std::forward<Arg>(arg)...);
		}

		///	@brief	记录错误
		template <typename... Arg>
		void LogErr(ncTStr content, Arg &&... arg)
		{
			Log(Err, content, std::forward<Arg>(arg)...);
		}

		///	@brief	记录警告
		template <typename... Arg>
		void LogWarn(ncTStr content, Arg &&... arg)
		{
			Log(Warn, content, std::forward<Arg>(arg)...);
		}

		///	@brief	记录
		template <typename... Arg>
		void Log(nuInt type, ncTStr content, Arg &&... arg)
		{
			UpdateLog(type, natUtil::FormatString(content, std::forward<Arg>(arg)...));
		}

		///	@brief	注册日志更新事件处理函数
		void RegisterLogUpdateEventFunc(natEventBus::EventListenerDelegate func);

	private:
		struct OutputToOStream
		{
			template <typename... RestChar_t>
			static void Impl(ncTStr str, std::basic_ostream<nChar>& currentOStream, std::basic_ostream<RestChar_t>&... _ostreams)
			{
				currentOStream << str << std::endl;
				Impl(str, _ostreams...);
			}

			template <typename... RestChar_t>
			static void Impl(ncTStr str, std::basic_ostream<nWChar>& currentOStream, std::basic_ostream<RestChar_t>&... _ostreams)
			{
				currentOStream << str << std::endl;
				Impl(str, _ostreams...);
			}

			static void Impl(ncTStr /*str*/)
			{
			}
		};

	public:
		template <typename... Char_t>
		void UseDefaultAction(natConsole& console, std::basic_ostream<Char_t>&... ostreams)
		{
			RegisterLogUpdateEventFunc([&console, &ostreams...](natEventBase& event)
			{
				auto&& eventLogUpdated(static_cast<EventLogUpdated&>(event));
				auto time = std::chrono::system_clock::to_time_t(eventLogUpdated.GetTime());
				tm timeStruct;
#ifdef _MSC_VER
				localtime_s(&timeStruct, &time);
#else
				timeStruct = *localtime(&time);
#endif
				auto logType = static_cast<LogType>(eventLogUpdated.GetLogType());
				auto logStr = natUtil::FormatString("[{0}] [{1}] {2}"_nv, std::put_time(&timeStruct, "%F %T"), GetDefaultLogTypeName(logType), eventLogUpdated.GetData());
				switch (logType)
				{
				case Msg:
				case Warn:
					console.WriteLine(logStr);
					OutputToOStream::Impl(logStr, ostreams...);
					break;
				case Err:
					console.WriteLineErr(logStr);
					OutputToOStream::Impl(logStr, ostreams...);
					break;
				default:
					break;
				}
			});
		}

		static ncTStr GetDefaultLogTypeName(LogType logtype);

	private:
		void UpdateLog(nuInt type, nTString&& log);
		natEventBus& m_EventBus;
	};
}
