#include "stdafx.h"
#include "natMisc.h"
#include "natException.h"

using namespace NatsuLib;

void detail_::NotConstructed()
{
	nat_Throw(natException, "Storage has not constructed."_nv);
}

void detail_::ValueNotAvailable()
{
	nat_Throw(natException, "There is no available value."_nv);
}
