#pragma once
#include "natConfig.h"
#include "natString.h"

namespace NatsuLib
{
	namespace Environment
	{
		///	@brief	��õ�ǰ�����µĻ��з�
		nStrView GetNewLine();

		////////////////////////////////////////////////////////////////////////////////
		///	@brief	����
		////////////////////////////////////////////////////////////////////////////////
		enum class Endianness
		{
			BigEndian,
			LittleEndian,
			MiddleEndian,	// ����֧�֣�ֻ�Ƿ����������
		};

		///	@brief	��õ�ǰ�����µĶ���
		Endianness GetEndianness();

		///	@brief	��û�������
		///	@param	name	����������key
		nString GetEnvironmentVar(nStrView name);

		///	@brief	���û�������
		///	@param	name	����������key
		///	@param	value	����������value
		void SetEnvironmentVar(nStrView name, nStrView value);
	}
}
