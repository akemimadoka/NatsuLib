#include "stdafx.h"
#include "natEnvironment.h"

using namespace NatsuLib;

nStrView Environment::GetNewLine()
{
#ifdef _WIN32
	return "\r\n"_nv;
#else
	return "\n"_nv;
#endif
}
