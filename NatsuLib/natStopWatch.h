////////////////////////////////////////////////////////////////////////////////
///	@file	natStopWatch.h
///	@brief	计时器
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natConfig.h"
#ifdef WIN32
#include <Windows.h>
#else
#include <chrono>
#endif

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@addtogroup	杂项
	///	@{

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	高精度停表类
	////////////////////////////////////////////////////////////////////////////////
	class natStopWatch final
	{
	public:
		natStopWatch();

		///	@brief	暂停
		void Pause();

		///	@brief	继续
		void Resume();

		///	@brief	重设
		void Reset();

		///	@brief	获得流逝时间
		///	@note	单位为秒
		nDouble GetElpased() const;
	private:
#ifdef WIN32
		LARGE_INTEGER	m_cFreq,	///< @brief	cpu频率
			m_cLast,	///< @brief	上一次时间
			m_cFixStart,///< @brief	暂停时用于修复的参数
			m_cFixAll;	///< @brief	暂停时用于修复的参数
#else
		std::chrono::high_resolution_clock::time_point m_Last, m_FixStart, m_FixAll;
#endif
	};

	///	@}
}
