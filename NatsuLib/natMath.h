////////////////////////////////////////////////////////////////////////////////
///	@file	natMath.h
///	@brief	NatsuLib数学相关
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natType.h"
#include "natException.h"
#include <type_traits>

////////////////////////////////////////////////////////////////////////////////
///	@addtogroup	NatsuLib数学
///	@brief		NatsuLib数学部分
///	@{

#include "natVec.h"
#include "natMat.h"
#include "natQuat.h"

#include "natTransform.h"
#include <algorithm>

template <typename T = nFloat>
struct natRect
{
	natVec2<T> a, b;

	natRect() = default;
	natRect(natVec2<T> const& A, natVec2<T> const& B) : a(A), b(B)
	{
	}
	natRect(T x1, T y1, T x2, T y2) : a(x1, y1), b(x2, y2)
	{
	}

	T GetWidth() const noexcept
	{
		T ret = b.x - a.x;
		return ret > T(0) ? ret : -ret;
	}

	T GetHeight() const noexcept
	{
		T ret = b.y - a.y;
		return ret > T(0) ? ret : -ret;
	}

	nBool Contain(natVec2<T> const& x) const noexcept
	{
		return (x.x >= a.x && x.x <= b.x && x.y >= a.y && x.y <= b.y);
	}

	nBool Contain(natVec2<T> const& x, T R) const noexcept
	{
		return (x.x + R >= a.x && x.x - R <= b.x && x.y + R >= a.y && x.y - R <= b.y);
	}

	natVec2<T> GetCenter() const noexcept
	{
		return (a + b) * 0.5;
	}

	nBool Intersect(natRect const& Rect, natRect* Out = nullptr) const noexcept
	{
		natRect tRet(std::max(a.x, Rect.a.x), std::max(a.y, Rect.a.y), std::min(b.x, Rect.b.x), std::min(b.y, Rect.b.y));

		if (tRet.a.x <= tRet.b.x && tRet.a.y <= tRet.b.y)
		{
			if (Out)
			{
				*Out = tRet;
			}
			return true;
		}
		return false;
	}
};

///	@}