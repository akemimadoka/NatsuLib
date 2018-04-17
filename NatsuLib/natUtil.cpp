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
};

std::wstring natUtil::MultibyteToUnicode(ncStr Str, nInt StrLength, nuInt CodePage)
{
	if (StrLength == 0)
	{
		return {};
	}

	const auto num = MultiByteToWideChar(CodePage, MB_ERR_INVALID_CHARS, Str, StrLength * sizeof(nChar), nullptr, 0);
	if (num == 0)
	{
		nat_Throw(natWinException, "MultiByteToWideChar failed."_nv);
	}

	std::wstring ret(static_cast<size_t>(num), 0);
	if (!MultiByteToWideChar(CodePage, MB_ERR_INVALID_CHARS, Str, StrLength * sizeof(nChar), &ret.front(), num))
	{
		nat_Throw(natWinException, "MultiByteToWideChar failed."_nv);
	}

	return ret;
}

std::string natUtil::WidecharToMultibyte(ncWStr Str, nInt StrLength, nuInt CodePage)
{
	if (StrLength == 0)
	{
		return {};
	}

	const DWORD flags = CodePage == CP_UTF8 || CodePage == 54936 ? WC_ERR_INVALID_CHARS : 0;
	SetLastError(ERROR_SUCCESS); // Workaround: wtf?
	auto num = WideCharToMultiByte(CodePage, flags, Str, StrLength, nullptr, 0, nullptr, FALSE);
	auto lastError = GetLastError();
	if (num == 0 || lastError)
	{
		nat_Throw(natWinException, lastError, "WideCharToMultiByte failed."_nv);
	}

	std::string ret(static_cast<size_t>(num), 0);
	num = WideCharToMultiByte(CodePage, flags, Str, StrLength, &ret.front(), num * sizeof(nChar), nullptr, FALSE);
	lastError = GetLastError();
	if (num == 0 || lastError)
	{
		nat_Throw(natWinException, lastError, "WideCharToMultiByte failed."_nv);
	}

	return ret;
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
	if (hRsrc)
	{
		const auto dwSize = SizeofResource(hInstance, hRsrc);
		if (dwSize)
		{
			const auto hGlobal = LoadResource(hInstance, hRsrc);
			if (hGlobal)
			{
				const auto pBuffer = static_cast<nData>(LockResource(hGlobal));
				if (pBuffer)
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
