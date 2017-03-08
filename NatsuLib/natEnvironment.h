#pragma once
#include "natConfig.h"
#include "natString.h"

namespace NatsuLib
{
	namespace Environment
	{
		nStrView GetNewLine();

		enum class Endianness
		{
			BigEndian,
			LittleEndian,
			MiddleEndian,	// 并不支持，只是放在这里而已
		};

		Endianness GetEndianness();

		nString GetEnvironmentVar(nStrView name);
		void SetEnvironmentVar(nStrView name, nStrView value);
	}
}
