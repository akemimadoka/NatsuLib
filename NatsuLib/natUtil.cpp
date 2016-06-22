#include "stdafx.h"
#include "natUtil.h"
#include "natException.h"

namespace natUtil
{

	enum : nuInt
	{
		FIXCHAR = 
#	ifdef UNICODE
		1u
#	else
		2u
#	endif
	};

	/*nTString FormatString(ncTStr lpStr, ...)
	{
		va_list vl;
		va_start(vl, lpStr);

		return FormatStringv(lpStr, vl);
	}

	nTString FormatStringv(ncTStr lpStr, const va_list vl)
	{
		int nLen;
		std::vector<nTChar> tBuf(lstrlen(lpStr) + 1u);

		try
		{
			do
			{
				tBuf.resize(tBuf.size() * 2u);
				nLen = _vsntprintf_s(tBuf.data(), tBuf.size() - 1, _TRUNCATE, lpStr, vl);
			} while (nLen < 0 || tBuf.size() - nLen <= FIXCHAR);
		}
		catch (std::bad_alloc&)
		{
			nat_Throw(natException, _T("Allocate memory failed"));
		}

		return tBuf.data();
	}*/

	std::wstring MultibyteToUnicode(ncStr Str, nuInt CodePage)
	{
		nInt Num = MultiByteToWideChar(CodePage, 0, Str, -1, nullptr, 0);
		std::vector<nWChar> tBuffer(Num);
		MultiByteToWideChar(CodePage, 0, Str, -1, tBuffer.data(), Num);

		return tBuffer.data();
	}

	std::string WidecharToMultibyte(ncWStr Str, nuInt CodePage)
	{
		nInt Num = WideCharToMultiByte(CodePage, 0, Str, -1, nullptr, 0, nullptr, FALSE);
		std::vector<nChar> tBuffer(Num);
		WideCharToMultiByte(CodePage, 0, Str, -1, tBuffer.data(), Num, nullptr, FALSE);

		return tBuffer.data();
	}

	nTString GetResourceString(DWORD ResourceID, HINSTANCE hInstance)
	{
		int nLen;
		std::vector<nTChar> tBuf(16u);

		try
		{
			do
			{
				tBuf.resize(tBuf.size() * 2u);
				nLen = LoadString(hInstance, ResourceID, tBuf.data(), tBuf.size() - 1u);
			} while (nLen < 0 || tBuf.size() - nLen <= FIXCHAR);
		}
		catch (std::bad_alloc&)
		{
			nat_Throw(natException, _T("Allocate memory failed"));
		}

		return tBuf.data();
	}

	std::vector<nByte> GetResourceData(DWORD ResourceID, ncTStr lpType, HINSTANCE hInstance)
	{
		HRSRC hRsrc = FindResource(hInstance, MAKEINTRESOURCE(ResourceID), lpType);
		if (hRsrc != NULL)
		{
			DWORD dwSize = SizeofResource(hInstance, hRsrc);
			if (dwSize != 0ul)
			{
				HGLOBAL hGlobal = LoadResource(hInstance, hRsrc);
				if (hGlobal != NULL)
				{
					nData pBuffer = static_cast<nData>(LockResource(hGlobal));
					if (pBuffer != nullptr)
					{
						return std::vector<nByte>(pBuffer, pBuffer + dwSize);
					}
				}
			}
		}

		return std::vector<nByte>();
	}
}