#pragma once
#include "natConfig.h"
#include "natString.h"

namespace NatsuLib
{
	namespace Environment
	{
		///	@brief	获得当前环境下的换行符
		nStrView GetNewLine();

		////////////////////////////////////////////////////////////////////////////////
		///	@brief	端序
		////////////////////////////////////////////////////////////////////////////////
		enum class Endianness
		{
			BigEndian,
			LittleEndian,
			MiddleEndian,	// 并不支持，只是放在这里而已
		};

		///	@brief	获得当前环境下的端序
		Endianness GetEndianness();

		///	@brief	获得环境变量
		///	@param	name	环境变量的key
		nString GetEnvironmentVar(nStrView name);

		///	@brief	设置环境变量
		///	@param	name	环境变量的key
		///	@param	value	环境变量的value
		void SetEnvironmentVar(nStrView name, nStrView value);
	}
}
