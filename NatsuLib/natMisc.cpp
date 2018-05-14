#include "stdafx.h"
#include "natMisc.h"
#include "natException.h"

using namespace NatsuLib;

void detail_::NotConstructed()
{
	nat_Throw(natErrException, NatErr::NatErr_IllegalState, u8"Storage has not constructed."_nv);
}

void detail_::ValueNotAvailable()
{
	nat_Throw(natErrException, NatErr::NatErr_IllegalState, u8"There is no available value."_nv);
}

void detail_::AlreadyInitialized()
{
	nat_Throw(natErrException, NatErr::NatErr_IllegalState, u8"Value has been already initialized"_nv);
}
