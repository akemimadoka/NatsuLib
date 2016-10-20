#include "stdafx.h"
#include "natStopWatch.h"

using namespace NatsuLib;

#ifdef _WIN32
natStopWatch::natStopWatch()
	: m_cFreq{0}, m_cLast{0}, m_cFixStart{0}, m_cFixAll{0}
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

nDouble natStopWatch::GetElpased() const
{
	LARGE_INTEGER tNow;
	QueryPerformanceCounter(&tNow);
	return static_cast<nDouble>(tNow.QuadPart - m_cLast.QuadPart - m_cFixAll.QuadPart) / static_cast<nDouble>(m_cFreq.QuadPart);
}
#else
natStopWatch::natStopWatch()
{
}

void natStopWatch::Pause()
{
	m_FixStart = std::chrono::high_resolution_clock::now();
}

void natStopWatch::Resume()
{
	m_FixAll += std::chrono::high_resolution_clock::now() - m_FixStart;
}

void natStopWatch::Reset()
{
	m_FixAll -= m_FixAll.time_since_epoch();
	m_Last = std::chrono::high_resolution_clock::now();
}

nDouble natStopWatch::GetElpased() const
{
	return static_cast<nDouble>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Last - m_FixAll.time_since_epoch()).count());
}
#endif // _WIN32
