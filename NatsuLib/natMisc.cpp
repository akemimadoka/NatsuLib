#include "stdafx.h"
#include "natMisc.h"
#include "natException.h"

using namespace NatsuLib;

void detail_::NotConstructed()
{
	nat_Throw(natException, _T("Storage has not constructed."));
}

void detail_::ValueNotAvailable()
{
	nat_Throw(natException, _T("There is no available value."));
}
