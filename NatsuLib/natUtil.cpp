#include "stdafx.h"
#include "natUtil.h"
#include "natException.h"

#include <locale>
#include <codecvt>

using namespace NatsuLib;
using namespace natUtil;

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

std::wstring natUtil::C2Wstr(ncStr str, size_t n)
{
	return std::wstring_convert<std::codecvt_utf8<nWChar>> {}.from_bytes(str, str + n);
}

std::wstring natUtil::C2Wstr(std::string const& str)
{
	return C2Wstr(str.c_str(), str.size());
}

std::wstring natUtil::C2Wstr(ncStr str)
{
	return C2Wstr(str, std::char_traits<nChar>::length(str));
}

std::string natUtil::W2Cstr(ncWStr str, size_t n)
{
	return std::wstring_convert<std::codecvt_utf8<nWChar>> {}.to_bytes(str, str + n);
}

std::string natUtil::W2Cstr(std::wstring const& str)
{
	return W2Cstr(str.c_str(), str.size());
}

std::string natUtil::W2Cstr(ncWStr str)
{
	return W2Cstr(str, std::char_traits<nWChar>::length(str));
}

#ifdef UNICODE
nTString natUtil::ToTString(ncStr str, size_t n)
{
	return C2Wstr(str, n);
}

nTString natUtil::ToTString(std::string const& str)
{
	return C2Wstr(str);
}

nTString natUtil::ToTString(ncStr str)
{
	return C2Wstr(str);
}

nTString natUtil::ToTString(ncWStr str, size_t n)
{
	return{ str, n };
}

nTString natUtil::ToTString(std::wstring const& str)
{
	return ToTString(str.c_str(), str.size());
}

nTString natUtil::ToTString(ncWStr str)
{
	return ToTString(str, std::char_traits<nWChar>::length(str));
}
#else
nTString natUtil::ToTString(ncStr str, size_t n)
{
	return{ str, n };
}
nTString natUtil::ToTString(std::string const& str)
{
	return ToTString(str.c_str(), str.size());
}
nTString natUtil::ToTString(ncStr str)
{
	return ToTString(str, std::char_traits<nChar>::length(str));
}
nTString natUtil::ToTString(ncWStr str, size_t n)
{
	return W2Cstr(str, n);
}
nTString natUtil::ToTString(std::wstring const& str)
{
	return W2Cstr(str);
}
nTString natUtil::ToTString(ncWStr str)
{
	return W2Cstr(str);
}
#endif

#ifdef _WIN32
std::wstring natUtil::MultibyteToUnicode(ncStr Str, nuInt CodePage)
{
	auto Num = MultiByteToWideChar(CodePage, 0, Str, -1, nullptr, 0);
	std::vector<nWChar> tBuffer(static_cast<size_t>(Num));
	MultiByteToWideChar(CodePage, 0, Str, -1, tBuffer.data(), Num);

	return tBuffer.data();
}

std::string natUtil::WidecharToMultibyte(ncWStr Str, nuInt CodePage)
{
	auto Num = WideCharToMultiByte(CodePage, 0, Str, -1, nullptr, 0, nullptr, FALSE);
	std::vector<nChar> tBuffer(static_cast<size_t>(Num));
	WideCharToMultiByte(CodePage, 0, Str, -1, tBuffer.data(), Num, nullptr, FALSE);

	return tBuffer.data();
}

nTString natUtil::GetResourceString(DWORD ResourceID, HINSTANCE hInstance)
{
	int nLen;
	std::vector<nTChar> tBuf(16u);

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
		nat_Throw(natException, _T("Allocate memory failed"));
	}

	return tBuf.data();
}

std::vector<nByte> natUtil::GetResourceData(DWORD ResourceID, ncTStr lpType, HINSTANCE hInstance)
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

	nat_Throw(natException, _T("No such resource."));
}
#else
// TODO
#endif
