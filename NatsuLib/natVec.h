////////////////////////////////////////////////////////////////////////////////
///	@file	natVec.h
///	@brief	NatsuLib向量相关
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natType.h"
#include "natException.h"
#include <type_traits>

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@addtogroup	NatsuLib数学
	///	@brief		NatsuLib数学部分
	///	@{

#define OPERATORSCALAR(op) template <typename U>\
	natVec2& operator##op(U const& Scalar)\
	{\
		x op static_cast<T>(Scalar);\
		y op static_cast<T>(Scalar);\
		return *this;\
	}

#define OPERATORSELF(op) template <typename U>\
	natVec2& operator##op(natVec2<U> const& v)\
	{\
		x op static_cast<T>(v.x);\
		y op static_cast<T>(v.y);\
		return *this;\
	}

#define TOPERATORSCALARNML(op) template <typename T, typename U>\
auto operator##op(natVec2<T> const& v, U const& Scalar)\
{\
	return natVec2<decltype(v.x op Scalar)>(v.x op Scalar, v.y op Scalar);\
}

#define TOPERATORSCALARNM(op) template <typename T, typename U>\
auto operator##op(U const& Scalar, natVec2<T> const& v)\
{\
	return natVec2<decltype(Scalar op v.x)>(Scalar op v.x, Scalar op v.y);\
}

#define TOPERATORSELFNM(op) template <typename T, typename U>\
auto operator##op(natVec2<T> const& v1, natVec2<U> const& v2)\
{\
	return natVec2<decltype(v1.x op v2.x)>(v1.x op v2.x, v1.y op v2.y);\
}

	struct natVec {};

	template <typename T>
	struct natVec3;

	template <typename T>
	struct natVec4;

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	2维向量
	////////////////////////////////////////////////////////////////////////////////
	template <typename T = nFloat>
	struct natVec2 final
		: natVec
	{
		typedef T type;

		union
		{
			struct
			{
				T x, y;
			};
			struct
			{
				T r, g;
			};
		};

		static constexpr nuInt length() noexcept
		{
			return 2u;
		}

		T const& operator[](nuInt i) const
		{
			if (i >= length())
			{
				nat_Throw(natException, _T("Out of range"));
			}

			return (&x)[i];
		}

		T& operator[](nuInt i)
		{
			return const_cast<T&>(static_cast<natVec2 const*>(this)->operator[](i));
		}

		template <typename R>
		natVec2<R> call(R(*func)(T)) const
		{
			return natVec2<R>(func(x), func(y));
		}

		natVec2()
			: x(0), y(0)
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		explicit natVec2(T const& Scalar)
			: x(Scalar), y(Scalar)
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		natVec2(T const& S1, T const& S2)
			: x(S1), y(S2)
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		template <typename U>
		explicit natVec2(const U* S)
			: x(static_cast<T>(S[0])), y(static_cast<T>(S[1]))
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		template <typename U>
		explicit natVec2(natVec2<U> && v)
			: x(static_cast<T&&>(v.x)), y(static_cast<T&&>(v.y))
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		template <typename U>
		explicit natVec2(natVec2<U> const& v)
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y))
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		natVec2(natVec2 const& v) = default;
		natVec2(natVec2 && v) = default;

		template <typename U>
		explicit natVec2(natVec3<U> const& v);

		template <typename U>
		explicit natVec2(natVec4<U> const& v);

		natVec2& operator=(natVec2 const& v) = default;
		natVec2& operator=(natVec2 && v) = default;

		template <typename U>
		natVec2& operator=(natVec2<U> && v)
		{
			x = static_cast<T&&>(v.x);
			y = static_cast<T&&>(v.y);

			return *this;
		}

		natVec2& operator++()
		{
			++x; ++y;
			return *this;
		}

		natVec2& operator--()
		{
			--x; --y;
			return *this;
		}

		natVec2 operator++(int)
		{
			natVec2 tResult(*this);
			++*this;
			return tResult;
		}

		natVec2 operator--(int)
		{
			natVec2 tResult(*this);
			--*this;
			return tResult;
		}

		natVec2 operator-() const
		{
			return natVec2(-x, -y);
		}

		natVec2 operator~() const
		{
			return natVec2(~x, ~y);
		}

		template <typename U>
		nBool operator==(natVec2<U> const& v) const
		{
			return (x == v.x) && (y == v.y);
		}

		template <typename U>
		nBool operator!=(natVec2<U> const& v) const
		{
			return !(*this == v);
		}

		OPERATORSCALAR(= );
		OPERATORSELF(= );

		OPERATORSCALAR(+= );
		OPERATORSELF(+= );

		OPERATORSCALAR(-= );
		OPERATORSELF(-= );

		OPERATORSCALAR(*= );
		OPERATORSELF(*= );

		OPERATORSCALAR(/= );
		OPERATORSELF(/= );

		OPERATORSCALAR(%= );
		OPERATORSELF(%= );

		OPERATORSCALAR(&= );
		OPERATORSELF(&= );

		OPERATORSCALAR(|= );
		OPERATORSELF(|= );

		OPERATORSCALAR(^= );
		OPERATORSELF(^= );

		OPERATORSCALAR(<<= );
		OPERATORSELF(<<= );

		OPERATORSCALAR(>>= );
		OPERATORSELF(>>= );
	};

	TOPERATORSCALARNML(+);
	TOPERATORSCALARNML(-);
	TOPERATORSCALARNML(*);
	TOPERATORSCALARNML(/ );
	TOPERATORSCALARNML(%);
	TOPERATORSCALARNML(&);
	TOPERATORSCALARNML(| );
	TOPERATORSCALARNML(^);
	TOPERATORSCALARNML(<< );
	TOPERATORSCALARNML(>> );

	TOPERATORSCALARNM(+);
	TOPERATORSCALARNM(-);
	TOPERATORSCALARNM(*);
	TOPERATORSCALARNM(/ );
	TOPERATORSCALARNM(%);
	TOPERATORSCALARNM(&);
	TOPERATORSCALARNM(| );
	TOPERATORSCALARNM(^);
	TOPERATORSCALARNM(<< );
	TOPERATORSCALARNM(>> );

	TOPERATORSELFNM(+);
	TOPERATORSELFNM(-);
	TOPERATORSELFNM(*);
	TOPERATORSELFNM(/ );
	TOPERATORSELFNM(%);
	TOPERATORSELFNM(&);
	TOPERATORSELFNM(| );
	TOPERATORSELFNM(^);
	TOPERATORSELFNM(<< );
	TOPERATORSELFNM(>> );

#ifdef OPERATORSCALAR
#	undef OPERATORSCALAR
#endif

#ifdef OPERATORSELF
#	undef OPERATORSELF
#endif

#ifdef TOPERATORSCALARNML
#	undef TOPERATORSCALARNML
#endif

#ifdef TOPERATORSCALARNM
#	undef TOPERATORSCALARNM
#endif

#ifdef TOPERATORSELFNM
#	undef TOPERATORSELFNM
#endif

#define OPERATORSCALAR(op) template <typename U>\
	natVec3& operator##op(U const& Scalar)\
	{\
		x op static_cast<T>(Scalar);\
		y op static_cast<T>(Scalar);\
		z op static_cast<T>(Scalar);\
		return *this;\
	}

#define OPERATORSELF(op) template <typename U>\
	natVec3& operator##op(natVec3<U> const& v)\
	{\
		x op static_cast<T>(v.x);\
		y op static_cast<T>(v.y);\
		z op static_cast<T>(v.z);\
		return *this;\
	}

#define TOPERATORSCALARNML(op) template <typename T, typename U>\
auto operator##op(natVec3<T> const& v, U const& Scalar)\
{\
	return natVec3<decltype(v.x op Scalar)>(v.x op Scalar, v.y op Scalar, v.z op Scalar);\
}

#define TOPERATORSCALARNM(op) template <typename T, typename U>\
auto operator##op(U const& Scalar, natVec3<T> const& v)\
{\
	return natVec3<decltype(Scalar op v.x)>(Scalar op v.x, Scalar op v.y, Scalar op v.z);\
}

#define TOPERATORSELFNM(op) template <typename T, typename U>\
auto operator##op(natVec3<T> const& v1, natVec3<U> const& v2)\
{\
	return natVec3<decltype(v1.x op v2.x)>(v1.x op v2.x, v1.y op v2.y, v1.z op v2.z);\
}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	3维向量
	////////////////////////////////////////////////////////////////////////////////
	template <typename T = nFloat>
	struct natVec3 final
		: natVec
	{
		typedef T type;

		union
		{
			struct
			{
				T x, y, z;
			};
			struct
			{
				T r, g, b;
			};
		};

		static constexpr nuInt length() noexcept
		{
			return 3u;
		}

		T const& operator[](nuInt i) const
		{
			if (i >= length())
			{
				nat_Throw(natException, _T("Out of range"));
			}

			return *(&x + i);
		}

		T& operator[](nuInt i)
		{
			return const_cast<T&>(static_cast<natVec3 const*>(this)->operator[](i));
		}

		template <typename R>
		natVec3<R> call(R(*func)(T)) const
		{
			return natVec3<R>(func(x), func(y), func(z));
		}

		natVec3()
			: x(0), y(0), z(0)
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		explicit natVec3(T const& Scalar)
			: x(Scalar), y(Scalar), z(Scalar)
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		natVec3(T const& s1, T const& s2, T const& s3)
			: x(s1), y(s2), z(s3)
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		template <typename U>
		explicit natVec3(const U* S)
			: x(static_cast<T>(S[0])), y(static_cast<T>(S[1])), z(static_cast<T>(S[2]))
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		template <typename U>
		explicit natVec3(natVec3<U> && v)
			: x(static_cast<T&&>(v.x)), y(static_cast<T&&>(v.y)), z(static_cast<T&&>(v.z))
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		template <typename U>
		explicit natVec3(natVec3<U> const& v)
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z))
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		natVec3(natVec3 const& v) = default;
		natVec3(natVec3 && v) = default;

		template <typename U, typename V>
		natVec3(natVec2<U> const& v, V const& a)
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(a))
		{
		}

		template <typename U, typename V>
		natVec3(U const& a, natVec2<V> const& v)
			: x(static_cast<T>(a)), y(static_cast<T>(v.x)), z(static_cast<T>(v.y))
		{
		}

		template <typename U>
		explicit natVec3(natVec4<U> const& v);

		natVec3& operator=(natVec3 const& v) = default;
		natVec3& operator=(natVec3 && v) = default;

		template <typename U>
		natVec3& operator=(natVec3<U> && v)
		{
			x = static_cast<T&&>(v.x);
			y = static_cast<T&&>(v.y);
			z = static_cast<T&&>(v.z);

			return *this;
		}

		natVec3& operator++()
		{
			++x; ++y; ++z;
			return *this;
		}

		natVec3& operator--()
		{
			--x; --y; --z;
			return *this;
		}

		natVec3 operator++(int)
		{
			natVec3 tResult(*this);
			++*this;
			return tResult;
		}

		natVec3 operator--(int)
		{
			natVec3 tResult(*this);
			--*this;
			return tResult;
		}

		natVec3 operator-() const
		{
			return natVec3(-x, -y, -z);
		}

		natVec3 operator~() const
		{
			return natVec3(~x, ~y, ~z);
		}

		template <typename U>
		nBool operator==(natVec3<U> const& v) const
		{
			return (x == v.x) && (y == v.y) && (z == v.z);
		}

		template <typename U>
		nBool operator!=(natVec3<U> const& v) const
		{
			return !(*this == v);
		}

		OPERATORSCALAR(= );
		OPERATORSELF(= );

		OPERATORSCALAR(+= );
		OPERATORSELF(+= );

		OPERATORSCALAR(-= );
		OPERATORSELF(-= );

		OPERATORSCALAR(*= );
		OPERATORSELF(*= );

		OPERATORSCALAR(/= );
		OPERATORSELF(/= );

		OPERATORSCALAR(%= );
		OPERATORSELF(%= );

		OPERATORSCALAR(&= );
		OPERATORSELF(&= );

		OPERATORSCALAR(|= );
		OPERATORSELF(|= );

		OPERATORSCALAR(^= );
		OPERATORSELF(^= );

		OPERATORSCALAR(<<= );
		OPERATORSELF(<<= );

		OPERATORSCALAR(>>= );
		OPERATORSELF(>>= );
	};

	TOPERATORSCALARNML(+);
	TOPERATORSCALARNML(-);
	TOPERATORSCALARNML(*);
	TOPERATORSCALARNML(/ );
	TOPERATORSCALARNML(%);
	TOPERATORSCALARNML(&);
	TOPERATORSCALARNML(| );
	TOPERATORSCALARNML(^);
	TOPERATORSCALARNML(<< );
	TOPERATORSCALARNML(>> );

	TOPERATORSCALARNM(+);
	TOPERATORSCALARNM(-);
	TOPERATORSCALARNM(*);
	TOPERATORSCALARNM(/ );
	TOPERATORSCALARNM(%);
	TOPERATORSCALARNM(&);
	TOPERATORSCALARNM(| );
	TOPERATORSCALARNM(^);
	TOPERATORSCALARNM(<< );
	TOPERATORSCALARNM(>> );

	TOPERATORSELFNM(+);
	TOPERATORSELFNM(-);
	TOPERATORSELFNM(*);
	TOPERATORSELFNM(/ );
	TOPERATORSELFNM(%);
	TOPERATORSELFNM(&);
	TOPERATORSELFNM(| );
	TOPERATORSELFNM(^);
	TOPERATORSELFNM(<< );
	TOPERATORSELFNM(>> );

#ifdef OPERATORSCALAR
#	undef OPERATORSCALAR
#endif

#ifdef OPERATORSELF
#	undef OPERATORSELF
#endif

#ifdef TOPERATORSCALARNML
#	undef TOPERATORSCALARNML
#endif

#ifdef TOPERATORSCALARNM
#	undef TOPERATORSCALARNM
#endif

#ifdef TOPERATORSELFNM
#	undef TOPERATORSELFNM
#endif

#define OPERATORSCALAR(op) template <typename U>\
	natVec4& operator##op(U const& Scalar)\
	{\
		x op static_cast<T>(Scalar);\
		y op static_cast<T>(Scalar);\
		z op static_cast<T>(Scalar);\
		w op static_cast<T>(Scalar);\
		return *this;\
	}

#define OPERATORSELF(op) template <typename U>\
	natVec4& operator##op(natVec4<U> const& v)\
	{\
		x op static_cast<T>(v.x);\
		y op static_cast<T>(v.y);\
		z op static_cast<T>(v.z);\
		w op static_cast<T>(v.w);\
		return *this;\
	}

#define TOPERATORSCALARNML(op) template <typename T, typename U>\
auto operator##op(natVec4<T> const& v, U const& Scalar)\
{\
	return natVec4<decltype(v.x op Scalar)>(v.x op Scalar, v.y op Scalar, v.z op Scalar, v.w op Scalar);\
}

#define TOPERATORSCALARNM(op) template <typename T, typename U>\
auto operator##op(U const& Scalar, natVec4<T> const& v)\
{\
	return natVec4<decltype(Scalar op v.x)>(Scalar op v.x, Scalar op v.y, Scalar op v.z, Scalar op v.w);\
}

#define TOPERATORSELFNM(op) template <typename T, typename U>\
auto operator##op(natVec4<T> const& v1, natVec4<U> const& v2)\
{\
	return natVec4<decltype(v1.x op v2.x)>(v1.x op v2.x, v1.y op v2.y, v1.z op v2.z, v1.w op v2.w);\
}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	4维向量
	////////////////////////////////////////////////////////////////////////////////
	template <typename T = nFloat>
	struct natVec4 final
		: natVec
	{
		typedef T type;

		union
		{
			struct
			{
				T x, y, z, w;
			};
			struct
			{
				T r, g, b, a;
			};
		};

		static constexpr nuInt length() noexcept
		{
			return 4u;
		}

		T const& operator[](nuInt i) const
		{
			if (i >= length())
			{
				nat_Throw(natException, _T("Out of range"));
			}

			return (&x)[i];
		}

		T& operator[](nuInt i)
		{
			return const_cast<T&>(static_cast<natVec4 const*>(this)->operator[](i));
		}

		template <typename R>
		natVec4<R> call(R(*func)(T)) const
		{
			return natVec4<R>(func(x), func(y), func(z), func(w));
		}

		natVec4()
			: x(0), y(0), z(0), w(0)
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		explicit natVec4(T const& Scalar)
			: x(Scalar), y(Scalar), z(Scalar), w(Scalar)
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		natVec4(T const& s1, T const& s2, T const& s3, T const& s4)
			: x(s1), y(s2), z(s3), w(s4)
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		template <typename U>
		explicit natVec4(const U* S)
			: x(static_cast<T>(S[0])), y(static_cast<T>(S[1])), z(static_cast<T>(S[2])), w(static_cast<T>(S[3]))
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		template <typename U>
		explicit natVec4(natVec4<U> && v)
			: x(static_cast<T&&>(v.x)), y(static_cast<T&&>(v.y)), z(static_cast<T&&>(v.z)), w(static_cast<T&&>(v.w))
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		template <typename U>
		explicit natVec4(natVec4<U> const& v)
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)), w(static_cast<T>(v.w))
		{
			static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
		}

		natVec4(natVec4 const& v) = default;
		natVec4(natVec4 && v) = default;

		template <typename U, typename V, typename W>
		natVec4(natVec2<U> const& v, V const& a, W const& b)
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(a)), w(static_cast<T>(b))
		{
		}

		template <typename U, typename V, typename W>
		natVec4(U const& a, natVec2<V> const& v, W const& b)
			: x(static_cast<T>(a)), y(static_cast<T>(v.x)), z(static_cast<T>(v.y)), w(static_cast<T>(b))
		{
		}

		template <typename U, typename V, typename W>
		natVec4(U const& a, V const& b, natVec2<W> const& v)
			: x(static_cast<T>(a)), y(static_cast<T>(b)), z(static_cast<T>(v.x)), w(static_cast<T>(v.y))
		{
		}

		template <typename U, typename V>
		natVec4(natVec3<U> const& v, V const& a)
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)), w(static_cast<T>(a))
		{
		}

		template <typename U, typename V>
		natVec4(U const& a, natVec3<V> const& v)
			: x(static_cast<T>(a)), y(static_cast<T>(v.x)), z(static_cast<T>(v.y)), w(static_cast<T>(v.z))
		{
		}

		natVec4& operator=(natVec4 const& v) = default;
		natVec4& operator=(natVec4 && v) = default;

		template <typename U>
		natVec4& operator=(natVec4<U> && v)
		{
			x = static_cast<T&&>(v.x);
			y = static_cast<T&&>(v.y);
			z = static_cast<T&&>(v.z);
			w = static_cast<T&&>(v.w);

			return *this;
		}

		natVec4& operator++()
		{
			++x; ++y; ++z; ++w;
			return *this;
		}

		natVec4& operator--()
		{
			--x; --y; --z; --w;
			return *this;
		}

		natVec4 operator++(int)
		{
			natVec4 tResult(*this);
			++*this;
			return tResult;
		}

		natVec4 operator--(int)
		{
			natVec4 tResult(*this);
			--*this;
			return tResult;
		}

		natVec4 operator-() const
		{
			return natVec4(-x, -y, -z, -w);
		}

		natVec4 operator~() const
		{
			return natVec4(~x, ~y, ~z, ~w);
		}

		template <typename U>
		nBool operator==(natVec4<U> const& v) const
		{
			return (x == v.x) && (y == v.y) && (z == v.z) && (w == v.w);
		}

		template <typename U>
		nBool operator!=(natVec4<U> const& v) const
		{
			return !(*this == v);
		}

		OPERATORSCALAR(= );
		OPERATORSELF(= );

		OPERATORSCALAR(+= );
		OPERATORSELF(+= );

		OPERATORSCALAR(-= );
		OPERATORSELF(-= );

		OPERATORSCALAR(*= );
		OPERATORSELF(*= );

		OPERATORSCALAR(/= );
		OPERATORSELF(/= );

		OPERATORSCALAR(%= );
		OPERATORSELF(%= );

		OPERATORSCALAR(&= );
		OPERATORSELF(&= );

		OPERATORSCALAR(|= );
		OPERATORSELF(|= );

		OPERATORSCALAR(^= );
		OPERATORSELF(^= );

		OPERATORSCALAR(<<= );
		OPERATORSELF(<<= );

		OPERATORSCALAR(>>= );
		OPERATORSELF(>>= );
	};

	TOPERATORSCALARNML(+);
	TOPERATORSCALARNML(-);
	TOPERATORSCALARNML(*);
	TOPERATORSCALARNML(/ );
	TOPERATORSCALARNML(%);
	TOPERATORSCALARNML(&);
	TOPERATORSCALARNML(| );
	TOPERATORSCALARNML(^);
	TOPERATORSCALARNML(<< );
	TOPERATORSCALARNML(>> );

	TOPERATORSCALARNM(+);
	TOPERATORSCALARNM(-);
	TOPERATORSCALARNM(*);
	TOPERATORSCALARNM(/ );
	TOPERATORSCALARNM(%);
	TOPERATORSCALARNM(&);
	TOPERATORSCALARNM(| );
	TOPERATORSCALARNM(^);
	TOPERATORSCALARNM(<< );
	TOPERATORSCALARNM(>> );

	TOPERATORSELFNM(+);
	TOPERATORSELFNM(-);
	TOPERATORSELFNM(*);
	TOPERATORSELFNM(/ );
	TOPERATORSELFNM(%);
	TOPERATORSELFNM(&);
	TOPERATORSELFNM(| );
	TOPERATORSELFNM(^);
	TOPERATORSELFNM(<< );
	TOPERATORSELFNM(>> );

#ifdef OPERATORSCALAR
#	undef OPERATORSCALAR
#endif

#ifdef OPERATORSELF
#	undef OPERATORSELF
#endif

#ifdef TOPERATORSCALARNML
#	undef TOPERATORSCALARNML
#endif

#ifdef TOPERATORSCALARNM
#	undef TOPERATORSCALARNM
#endif

#ifdef TOPERATORSELFNM
#	undef TOPERATORSELFNM
#endif

	template <typename T>
	template <typename U>
	natVec2<T>::natVec2(natVec3<U> const& v)
		: x(static_cast<T>(v.x)), y(static_cast<T>(v.y))
	{
	}

	template <typename T>
	template <typename U>
	natVec2<T>::natVec2(natVec4<U> const& v)
		: x(static_cast<T>(v.x)), y(static_cast<T>(v.y))
	{
	}

	template <typename T>
	template <typename U>
	natVec3<T>::natVec3(natVec4<U> const& v)
		: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z))
	{
	}

	///	@}
}
