#pragma once
#include "natStringUtil.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <vector>

namespace NatsuLib
{
	namespace natUtil
	{
#ifdef _WIN32
		enum : nuInt
		{
			DefaultCodePage = CP_INSTALLED
		};

		///	@brief	多字节转Unicode
		std::wstring MultibyteToUnicode(ncStr Str, nuInt CodePage = DefaultCodePage);
		///	@brief	宽字符转多字节
		std::string WidecharToMultibyte(ncWStr Str, nuInt CodePage = DefaultCodePage);

		///	@brief	获得资源字符串
		///	@param[in]	ResourceID	资源ID
		///	@param[in]	hInstance	实例句柄，默认为NULL
		nString GetResourceString(DWORD ResourceID, HINSTANCE hInstance = NULL);

		///	@brief	获得资源数据
		///	@param[in]	ResourceID	资源ID
		///	@param[in]	lpType		资源类型
		///	@param[in]	hInstance	实例句柄，默认为NULL
		std::vector<nByte> GetResourceData(DWORD ResourceID, LPCTSTR lpType, HINSTANCE hInstance = NULL);
#endif
	}
}
