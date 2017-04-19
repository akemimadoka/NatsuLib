#include "stdafx.h"
#include "natUtil.h"
#include "natException.h"

using namespace NatsuLib;
using namespace natUtil;

#ifdef _WIN32
enum : nuInt
{
	FIXCHAR =
#ifdef UNICODE
	1u
#else
	2u
#endif
	,
};

std::wstring natUtil::MultibyteToUnicode(ncStr Str, nInt StrLength, nuInt CodePage)
{
	if (StrLength == 0)
	{
		return {};
	}

	auto Num = MultiByteToWideChar(CodePage, MB_ERR_INVALID_CHARS, Str, StrLength * sizeof(nChar), nullptr, 0);
	if (Num == 0)
	{
		nat_Throw(natWinException, "MultiByteToWideChar failed."_nv);
	}

	std::wstring ret(static_cast<size_t>(Num), 0);
	if (!MultiByteToWideChar(CodePage, MB_ERR_INVALID_CHARS, Str, StrLength * sizeof(nChar), &ret.front(), Num))
	{
		nat_Throw(natWinException, "MultiByteToWideChar failed."_nv);
	}

	return move(ret);
}

std::string natUtil::WidecharToMultibyte(ncWStr Str, nInt StrLength, nuInt CodePage)
{
	if (StrLength == 0)
	{
		return {};
	}

	const DWORD flags = CodePage == CP_UTF8 || CodePage == 54936 ? WC_ERR_INVALID_CHARS : 0;
	DWORD lastError;
	SetLastError(ERROR_SUCCESS); // Workaround: wtf?
	auto Num = WideCharToMultiByte(CodePage, flags, Str, StrLength, nullptr, 0, nullptr, FALSE);
	lastError = GetLastError();
	if (Num == 0 || lastError)
	{
		nat_Throw(natWinException, lastError, "WideCharToMultiByte failed."_nv);
	}

	std::string ret(static_cast<size_t>(Num), 0);
	Num = WideCharToMultiByte(CodePage, flags, Str, StrLength, &ret.front(), Num * sizeof(nChar), nullptr, FALSE);
	lastError = GetLastError();
	if (Num == 0 || lastError)
	{
		nat_Throw(natWinException, lastError, "WideCharToMultiByte failed."_nv);
	}

	return move(ret);
}

nString natUtil::GetResourceString(DWORD ResourceID, HINSTANCE hInstance)
{
	int nLen;
	std::vector<TCHAR> tBuf(16u);

	try
	{
		do
		{
			tBuf.resize(tBuf.size() * 2u);
			nLen = LoadString(hInstance, ResourceID, tBuf.data(), static_cast<int>(tBuf.size() - 1));
		} while (nLen < 0 || tBuf.size() - nLen <= FIXCHAR);
	}
	catch (std::bad_alloc&)
	{
		nat_Throw(MemoryAllocFail);
	}

#ifdef UNICODE
	return WideStringView{ tBuf.data() };
#else
	return AnsiStringView{ tBuf.data() };
#endif
}

std::vector<nByte> natUtil::GetResourceData(DWORD ResourceID, LPCTSTR lpType, HINSTANCE hInstance)
{
	const auto hRsrc = FindResource(hInstance, MAKEINTRESOURCE(ResourceID), lpType);
	if (hRsrc != NULL)
	{
		const auto dwSize = SizeofResource(hInstance, hRsrc);
		if (dwSize != 0ul)
		{
			const auto hGlobal = LoadResource(hInstance, hRsrc);
			if (hGlobal != NULL)
			{
				const auto pBuffer = static_cast<nData>(LockResource(hGlobal));
				if (pBuffer != nullptr)
				{
					return std::vector<nByte>(pBuffer, pBuffer + dwSize);
				}
			}
		}
	}

	nat_Throw(natException, "No such resource."_nv);
}
#else
// TODO
#endif
