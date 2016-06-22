#include "stdafx.h"
#include "natStopWatch.h"

natStopWatch::natStopWatch()
{
	QueryPerformanceFrequency(&m_cFreq);
	Reset();
}

void natStopWatch::Pause()
{
	QueryPerformanceCounter(&m_cFixStart);
}

void natStopWatch::Resume()
{
	LARGE_INTEGER tNow;
	QueryPerformanceCounter(&tNow);
	m_cFixAll.QuadPart += tNow.QuadPart - m_cFixStart.QuadPart;
}

void natStopWatch::Reset()
{
	m_cFixAll.QuadPart = 0ll;
	QueryPerformanceCounter(&m_cLast);
}

double natStopWatch::GetElpased()
{
	LARGE_INTEGER tNow;
	QueryPerformanceCounter(&tNow);
	return static_cast<double>(tNow.QuadPart - m_cLast.QuadPart - m_cFixAll.QuadPart) / static_cast<double>(m_cFreq.QuadPart);
}