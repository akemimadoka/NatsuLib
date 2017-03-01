#pragma once

#include "natConfig.h"
#include "natRefObj.h"

namespace NatsuLib
{
	namespace RelationalOperator
	{
		////////////////////////////////////////////////////////////////////////////////
		///	@brief	比较接口
		///	@tparam	T	要比较的类型
		////////////////////////////////////////////////////////////////////////////////
		template <typename T>
		struct IComparable
			: natRefObj
		{
			 ///	@brief	表示本对象和另一对象的大小关系
			 ///	@param	other	要比较的对象
			 ///	@return	返回负数表示本对象比另一对象更小，0表示相等，正数表示更大，具体的值无需有特殊含义，仅用符号表示大小关系
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
		nBool operator>(IComparable<T> const& a, T const& b)
		{
			return a.CompareTo(b) > 0;
		}

		template <typename T>
		nBool operator<=(IComparable<T> const& a, T const& b)
		{
			return a.CompareTo(b) <= 0;
		}

		template <typename T>
		nBool operator>=(IComparable<T> const& a, T const& b)
		{
			return a.CompareTo(b) >= 0;
		}

		template <typename T>
		nBool operator<(T const& a, IComparable<T> const& b)
		{
			return b > a;
		}

		template <typename T>
		nBool operator>(T const& a, IComparable<T> const& b)
		{
			return b < a;
		}

		template <typename T>
		nBool operator<=(T const& a, IComparable<T> const& b)
		{
			return b >= a;
		}

		template <typename T>
		nBool operator>=(T const& a, IComparable<T> const& b)
		{
			return b <= a;
		}
	}
}
