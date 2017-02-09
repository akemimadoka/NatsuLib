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

Environment::Endianness Environment::GetEndianness()
{
	static constexpr nuShort s_TestUint{ 0x1234 };
	static const auto s_Endianness = reinterpret_cast<ncData>(&s_TestUint)[0] == 0x12 ? Endianness::BigEndian : Endianness::LittleEndian;
	return s_Endianness;
}
