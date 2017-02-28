#pragma once

#include "natConfig.h"
#include "natRefObj.h"

namespace NatsuLib
{
	namespace RelationalOperator
	{
		template <typename T>
		struct IComparable
			: natRefObj
		{
			 virtual nInt CompareTo(T const& other) const = 0;
		};

		template <typename T>
		nBool operator==(IComparable<T> const& a, T const& b)
		{
			return a.CompareTo(b) == 0;
		}

		template <typename T>
		nBool operator==(T const& a, IComparable<T> const& b)
		{
			return b == a;
		}

		template <typename T>
		nBool operator!=(IComparable<T> const& a, T const& b)
		{
			return a.CompareTo(b) != 0;
		}

		template <typename T>
		nBool operator!=(T const& a, IComparable<T> const& b)
		{
			return b != a;
		}

		template <typename T>
		nBool operator<(IComparable<T> const& a, T const& b)
		{
			return a.CompareTo(b) < 0;
		}

		template <typename T>
		nBool operator<(T const& a, IComparable<T> const& b)
		{
			return b < a;
		}

		template <typename T>
		nBool operator>(IComparable<T> const& a, T const& b)
		{
			return a.CompareTo(b) > 0;
		}

		template <typename T>
		nBool operator>(T const& a, IComparable<T> const& b)
		{
			return b < a;
		}

		template <typename T>
		nBool operator<=(IComparable<T> const& a, T const& b)
		{
			return a.CompareTo(b) <= 0;
		}

		template <typename T>
		nBool operator<=(T const& a, IComparable<T> const& b)
		{
			return b <= a;
		}

		template <typename T>
		nBool operator>=(IComparable<T> const& a, T const& b)
		{
			return a.CompareTo(b) >= 0;
		}

		template <typename T>
		nBool operator>=(T const& a, IComparable<T> const& b)
		{
			return b <= a;
		}
	}
}
