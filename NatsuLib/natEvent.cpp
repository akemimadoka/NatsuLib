#include "stdafx.h"
#include "natEvent.h"

using namespace NatsuLib;

natEventBase::~natEventBase()
{
}

nBool natEventBase::CanCancel() const noexcept
{
	return false;
}

void natEventBase::SetCancel(nBool value) noexcept
{
	if (CanCancel())
	{
		m_Canceled = value;
	}
}

nBool natEventBase::IsCanceled() const noexcept
{
	return m_Canceled;
}
