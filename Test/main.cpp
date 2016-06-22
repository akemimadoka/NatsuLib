#include <iostream>
#include <natUtil.h>

int main()
{
	std::locale::global(std::locale("", LC_CTYPE));
	std::wcout << natUtil::FormatString(_T("{0} {1}"), _T("Test"), 123) << std::endl;
}
