#include <iostream>
#include <natUtil.h>
#include <natMisc.h>

using namespace NatsuLib;

int main()
{
	try
	{
		std::locale::global(std::locale("", LC_CTYPE));
		std::wcout << natUtil::FormatString(_T("{ 1 } {0}"), _T("Test"), 123) << std::endl;

		std::vector<nTString> strvec;
		natUtil::split(_T("test 2333"), _T(" "), [&strvec](ncTStr str, size_t len)
		{
			strvec.emplace_back(str, len);
		});
		for (auto&& item : strvec)
		{
			std::wcout << item << std::endl;
		}

		int arr[] = { 1, 2, 3, 4, 5 };
		for (auto&& item : make_range(arr).pop_front().pop_back(2))
		{
			std::cout << item << std::endl;
		}
	}
	catch (natException& e)
	{
		std::wcerr << natUtil::FormatString(_T("Exception caught from {0},\nDescription: {1}"), e.GetSource(), e.GetDesc()) << std::endl;
	}

	system("pause");
}
