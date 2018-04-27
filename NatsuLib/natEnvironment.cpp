#include "stdafx.h"
#include "natEnvironment.h"
#include "natException.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <cstdlib>
#endif

using namespace NatsuLib;

namespace NatsuLib
{
	namespace detail_
	{

	}
}

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

nString Environment::GetEnvironmentVar(nStrView name)
{
#ifdef _WIN32
	if (name.empty())
	{
		nat_Throw(natErrException, NatErr_InvalidArg, "name should be a valid string."_nv);
	}

#ifdef _UNICODE
	WideString
#else
	AnsiString
#endif
		nameBuffer{ name }, retBuffer;

	const auto bufferSize = GetEnvironmentVariable(nameBuffer.data(), nullptr, 0);
	if (!bufferSize)
	{
		nat_Throw(natWinException, "GetEnvironmentVariable failed."_nv);
	}

	retBuffer.Resize(static_cast<size_t>(bufferSize));

	if (!GetEnvironmentVariable(nameBuffer.data(), retBuffer.data(), static_cast<DWORD>(retBuffer.size())))
	{
		nat_Throw(natWinException, "GetEnvironmentVariable failed."_nv);
	}

	return retBuffer;
#else
	return std::getenv(name.data());
#endif
}

void Environment::SetEnvironmentVar(nStrView name, nStrView value)
{
#ifdef _WIN32
#ifdef _UNICODE
	WideString
#else
	AnsiString
#endif
		nameBuffer{ name }, valueBuffer{ value };

	if (!SetEnvironmentVariable(nameBuffer.data(), valueBuffer.data()))
	{
		nat_Throw(natWinException, "SetEnvironmentVariable failed."_nv);
	}
#else
	setenv(name.data(), value.data(), 1);
#endif
}
