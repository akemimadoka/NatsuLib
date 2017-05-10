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
	natVec2& operator##op(U const& Scalar) noexcept\
	{\
		x op static_cast<T>(Scalar);\
		y op static_cast<T>(Scalar);\
		return *this;\
	}

#define OPERATORSELF(op) template <typename U>\
	natVec2& operator##op(natVec2<U> const& v) noexcept\
	{\
		x op static_cast<T>(v.x);\
		y op static_cast<T>(v.y);\
		return *this;\
	}

#define TOPERATORSCALARNML(op) template <typename T, typename U>\
auto operator##op(natVec2<T> const& v, U const& Scalar) noexcept\
{\
	return natVec2<decltype(v.x op Scalar)>(v.x op Scalar, v.y op Scalar);\
}

#define TOPERATORSCALARNM(op) template <typename T, typename U>\
auto operator##op(U const& Scalar, natVec2<T> const& v) noexcept\
{\
	return natVec2<decltype(Scalar op v.x)>(Scalar op v.x, Scalar op v.y);\
}

#define TOPERATORSELFNM(op) template <typename T, typename U>\
auto operator##op(natVec2<T> const& v1, natVec2<U> const& v2) noexcept\
{\
	return natVec2<decltype(v1.x op v2.x)>(v1.x op v2.x, v1.y op v2.y);\
}

	template <typename T>
	struct natVec3;

	template <typename T>
	struct natVec4;

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	2维向量
	////////////////////////////////////////////////////////////////////////////////
	template <typename T = nFloat>
	struct natVec2 final
	{
		static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
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

		T const& operator[](nuInt i) const noexcept
		{
			assert(i < length() && "Out of range");

			if (i == 0)
			{
				return x;
			}

			return y;
		}

		T& operator[](nuInt i) noexcept
		{
			return const_cast<T&>(static_cast<natVec2 const*>(this)->operator[](i));
		}

		template <typename R>
		natVec2<R> call(R(*func)(T)) const
		{
			return natVec2<R>(func(x), func(y));
		}

		constexpr natVec2() noexcept
			: x(0), y(0)
		{
		}

		constexpr explicit natVec2(T const& Scalar) noexcept
			: x(Scalar), y(Scalar)
		{
		}

		constexpr natVec2(T const& S1, T const& S2) noexcept
			: x(S1), y(S2)
		{
		}

		template <typename U>
		constexpr explicit natVec2(const U* S) noexcept
			: x(static_cast<T>(S[0])), y(static_cast<T>(S[1]))
		{
		}

		template <typename U>
		constexpr explicit natVec2(natVec2<U> && v) noexcept
			: x(static_cast<T&&>(v.x)), y(static_cast<T&&>(v.y))
		{
		}

		template <typename U>
		constexpr explicit natVec2(natVec2<U> const& v) noexcept
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y))
		{
		}

		constexpr natVec2(natVec2 const& v) noexcept = default;
		constexpr natVec2(natVec2 && v) noexcept = default;

		template <typename U>
		constexpr explicit natVec2(natVec3<U> const& v) noexcept;

		template <typename U>
		constexpr explicit natVec2(natVec4<U> const& v) noexcept;

		natVec2& operator=(natVec2 const& v) noexcept = default;
		natVec2& operator=(natVec2 && v) noexcept = default;

		template <typename U>
		natVec2& operator=(natVec2<U> && v) noexcept
		{
			x = static_cast<T&&>(v.x);
			y = static_cast<T&&>(v.y);

			return *this;
		}

		natVec2& operator++() noexcept
		{
			++x; ++y;
			return *this;
		}

		natVec2& operator--() noexcept
		{
			--x; --y;
			return *this;
		}

		natVec2 operator++(int) noexcept
		{
			natVec2 tResult(*this);
			++*this;
			return tResult;
		}

		natVec2 operator--(int) noexcept
		{
			natVec2 tResult(*this);
			--*this;
			return tResult;
		}

		natVec2 operator-() const noexcept
		{
			return natVec2(-x, -y);
		}

		natVec2 operator~() const noexcept
		{
			return natVec2(~x, ~y);
		}

		template <typename U>
		nBool operator==(natVec2<U> const& v) const noexcept
		{
			return (x == v.x) && (y == v.y);
		}

		template <typename U>
		nBool operator!=(natVec2<U> const& v) const noexcept
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
	natVec3& operator##op(U const& Scalar) noexcept\
	{\
		x op static_cast<T>(Scalar);\
		y op static_cast<T>(Scalar);\
		z op static_cast<T>(Scalar);\
		return *this;\
	}

#define OPERATORSELF(op) template <typename U>\
	natVec3& operator##op(natVec3<U> const& v) noexcept\
	{\
		x op static_cast<T>(v.x);\
		y op static_cast<T>(v.y);\
		z op static_cast<T>(v.z);\
		return *this;\
	}

#define TOPERATORSCALARNML(op) template <typename T, typename U>\
auto operator##op(natVec3<T> const& v, U const& Scalar) noexcept\
{\
	return natVec3<decltype(v.x op Scalar)>(v.x op Scalar, v.y op Scalar, v.z op Scalar);\
}

#define TOPERATORSCALARNM(op) template <typename T, typename U>\
auto operator##op(U const& Scalar, natVec3<T> const& v) noexcept\
{\
	return natVec3<decltype(Scalar op v.x)>(Scalar op v.x, Scalar op v.y, Scalar op v.z);\
}

#define TOPERATORSELFNM(op) template <typename T, typename U>\
auto operator##op(natVec3<T> const& v1, natVec3<U> const& v2) noexcept\
{\
	return natVec3<decltype(v1.x op v2.x)>(v1.x op v2.x, v1.y op v2.y, v1.z op v2.z);\
}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	3维向量
	////////////////////////////////////////////////////////////////////////////////
	template <typename T = nFloat>
	struct natVec3 final
	{
		static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
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

		T const& operator[](nuInt i) const noexcept
		{
			assert(i < length() && "Out of range");

			if (i == 0)
			{
				return x;
			}

			if (i == 1)
			{
				return y;
			}

			return z;
		}

		T& operator[](nuInt i) noexcept
		{
			return const_cast<T&>(static_cast<natVec3 const*>(this)->operator[](i));
		}

		template <typename R>
		natVec3<R> call(R(*func)(T)) const
		{
			return natVec3<R>(func(x), func(y), func(z));
		}

		constexpr natVec3() noexcept
			: x(0), y(0), z(0)
		{
		}

		constexpr explicit natVec3(T const& Scalar) noexcept
			: x(Scalar), y(Scalar), z(Scalar)
		{
		}

		constexpr natVec3(T const& s1, T const& s2, T const& s3) noexcept
			: x(s1), y(s2), z(s3)
		{
		}

		template <typename U>
		constexpr explicit natVec3(const U* S) noexcept
			: x(static_cast<T>(S[0])), y(static_cast<T>(S[1])), z(static_cast<T>(S[2]))
		{
		}

		template <typename U>
		constexpr explicit natVec3(natVec3<U> && v) noexcept
			: x(static_cast<T&&>(v.x)), y(static_cast<T&&>(v.y)), z(static_cast<T&&>(v.z))
		{
		}

		template <typename U>
		constexpr explicit natVec3(natVec3<U> const& v) noexcept
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z))
		{
		}

		constexpr natVec3(natVec3 const& v) noexcept = default;
		constexpr natVec3(natVec3 && v) noexcept = default;

		template <typename U, typename V>
		constexpr natVec3(natVec2<U> const& v, V const& a) noexcept
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(a))
		{
		}

		template <typename U, typename V>
		constexpr natVec3(U const& a, natVec2<V> const& v) noexcept
			: x(static_cast<T>(a)), y(static_cast<T>(v.x)), z(static_cast<T>(v.y))
		{
		}

		template <typename U>
		constexpr explicit natVec3(natVec4<U> const& v) noexcept;

		natVec3& operator=(natVec3 const& v) noexcept = default;
		natVec3& operator=(natVec3 && v) noexcept = default;

		template <typename U>
		natVec3& operator=(natVec3<U> && v) noexcept
		{
			x = static_cast<T&&>(v.x);
			y = static_cast<T&&>(v.y);
			z = static_cast<T&&>(v.z);

			return *this;
		}

		natVec3& operator++() noexcept
		{
			++x; ++y; ++z;
			return *this;
		}

		natVec3& operator--() noexcept
		{
			--x; --y; --z;
			return *this;
		}

		natVec3 operator++(int) noexcept
		{
			natVec3 tResult(*this);
			++*this;
			return tResult;
		}

		natVec3 operator--(int) noexcept
		{
			natVec3 tResult(*this);
			--*this;
			return tResult;
		}

		natVec3 operator-() const noexcept
		{
			return natVec3(-x, -y, -z);
		}

		natVec3 operator~() const noexcept
		{
			return natVec3(~x, ~y, ~z);
		}

		template <typename U>
		nBool operator==(natVec3<U> const& v) const noexcept
		{
			return (x == v.x) && (y == v.y) && (z == v.z);
		}

		template <typename U>
		nBool operator!=(natVec3<U> const& v) const noexcept
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
	natVec4& operator##op(U const& Scalar) noexcept\
	{\
		x op static_cast<T>(Scalar);\
		y op static_cast<T>(Scalar);\
		z op static_cast<T>(Scalar);\
		w op static_cast<T>(Scalar);\
		return *this;\
	}

#define OPERATORSELF(op) template <typename U>\
	natVec4& operator##op(natVec4<U> const& v) noexcept\
	{\
		x op static_cast<T>(v.x);\
		y op static_cast<T>(v.y);\
		z op static_cast<T>(v.z);\
		w op static_cast<T>(v.w);\
		return *this;\
	}

#define TOPERATORSCALARNML(op) template <typename T, typename U>\
auto operator##op(natVec4<T> const& v, U const& Scalar) noexcept\
{\
	return natVec4<decltype(v.x op Scalar)>(v.x op Scalar, v.y op Scalar, v.z op Scalar, v.w op Scalar);\
}

#define TOPERATORSCALARNM(op) template <typename T, typename U>\
auto operator##op(U const& Scalar, natVec4<T> const& v) noexcept\
{\
	return natVec4<decltype(Scalar op v.x)>(Scalar op v.x, Scalar op v.y, Scalar op v.z, Scalar op v.w);\
}

#define TOPERATORSELFNM(op) template <typename T, typename U>\
auto operator##op(natVec4<T> const& v1, natVec4<U> const& v2) noexcept\
{\
	return natVec4<decltype(v1.x op v2.x)>(v1.x op v2.x, v1.y op v2.y, v1.z op v2.z, v1.w op v2.w);\
}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	4维向量
	////////////////////////////////////////////////////////////////////////////////
	template <typename T = nFloat>
	struct natVec4 final
	{
		static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value, "T should be an arithmetic type or a class.");
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

		T const& operator[](nuInt i) const noexcept
		{
			assert(i < length() && "Out of range");

			if (i == 0)
			{
				return x;
			}

			if (i == 1)
			{
				return y;
			}

			if (i == 2)
			{
				return z;
			}
			
			return w;
		}

		T& operator[](nuInt i) noexcept
		{
			return const_cast<T&>(static_cast<natVec4 const*>(this)->operator[](i));
		}

		template <typename R>
		natVec4<R> call(R(*func)(T)) const
		{
			return natVec4<R>(func(x), func(y), func(z), func(w));
		}

		constexpr natVec4() noexcept
			: x(0), y(0), z(0), w(0)
		{
		}

		constexpr explicit natVec4(T const& Scalar) noexcept
			: x(Scalar), y(Scalar), z(Scalar), w(Scalar)
		{
		}

		constexpr natVec4(T const& s1, T const& s2, T const& s3, T const& s4) noexcept
			: x(s1), y(s2), z(s3), w(s4)
		{
		}

		template <typename U>
		constexpr explicit natVec4(const U* S) noexcept
			: x(static_cast<T>(S[0])), y(static_cast<T>(S[1])), z(static_cast<T>(S[2])), w(static_cast<T>(S[3]))
		{
		}

		template <typename U>
		constexpr explicit natVec4(natVec4<U> && v) noexcept
			: x(static_cast<T&&>(v.x)), y(static_cast<T&&>(v.y)), z(static_cast<T&&>(v.z)), w(static_cast<T&&>(v.w))
		{
		}

		template <typename U>
		constexpr explicit natVec4(natVec4<U> const& v) noexcept
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)), w(static_cast<T>(v.w))
		{
		}

		constexpr natVec4(natVec4 const& v) noexcept = default;
		constexpr natVec4(natVec4 && v) noexcept = default;

		template <typename U, typename V, typename W>
		constexpr natVec4(natVec2<U> const& v, V const& a, W const& b) noexcept
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(a)), w(static_cast<T>(b))
		{
		}

		template <typename U, typename V, typename W>
		constexpr natVec4(U const& a, natVec2<V> const& v, W const& b) noexcept
			: x(static_cast<T>(a)), y(static_cast<T>(v.x)), z(static_cast<T>(v.y)), w(static_cast<T>(b))
		{
		}

		template <typename U, typename V, typename W>
		constexpr natVec4(U const& a, V const& b, natVec2<W> const& v) noexcept
			: x(static_cast<T>(a)), y(static_cast<T>(b)), z(static_cast<T>(v.x)), w(static_cast<T>(v.y))
		{
		}

		template <typename U, typename V>
		constexpr natVec4(natVec3<U> const& v, V const& a) noexcept
			: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)), w(static_cast<T>(a))
		{
		}

		template <typename U, typename V>
		constexpr natVec4(U const& a, natVec3<V> const& v) noexcept
			: x(static_cast<T>(a)), y(static_cast<T>(v.x)), z(static_cast<T>(v.y)), w(static_cast<T>(v.z))
		{
		}

		natVec4& operator=(natVec4 const& v) noexcept = default;
		natVec4& operator=(natVec4 && v) noexcept = default;

		template <typename U>
		natVec4& operator=(natVec4<U> && v) noexcept
		{
			x = static_cast<T&&>(v.x);
			y = static_cast<T&&>(v.y);
			z = static_cast<T&&>(v.z);
			w = static_cast<T&&>(v.w);

			return *this;
		}

		natVec4& operator++() noexcept
		{
			++x; ++y; ++z; ++w;
			return *this;
		}

		natVec4& operator--() noexcept
		{
			--x; --y; --z; --w;
			return *this;
		}

		natVec4 operator++(int) noexcept
		{
			natVec4 tResult(*this);
			++*this;
			return tResult;
		}

		natVec4 operator--(int) noexcept
		{
			natVec4 tResult(*this);
			--*this;
			return tResult;
		}

		natVec4 operator-() const noexcept
		{
			return natVec4(-x, -y, -z, -w);
		}

		natVec4 operator~() const noexcept
		{
			return natVec4(~x, ~y, ~z, ~w);
		}

		template <typename U>
		nBool operator==(natVec4<U> const& v) const noexcept
		{
			return (x == v.x) && (y == v.y) && (z == v.z) && (w == v.w);
		}

		template <typename U>
		nBool operator!=(natVec4<U> const& v) const noexcept
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
	constexpr natVec2<T>::natVec2(natVec3<U> const& v) noexcept
		: x(static_cast<T>(v.x)), y(static_cast<T>(v.y))
	{
	}

	template <typename T>
	template <typename U>
	constexpr natVec2<T>::natVec2(natVec4<U> const& v) noexcept
		: x(static_cast<T>(v.x)), y(static_cast<T>(v.y))
	{
	}

	template <typename T>
	template <typename U>
	constexpr natVec3<T>::natVec3(natVec4<U> const& v) noexcept
		: x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z))
	{
	}

	template <typename T>
	struct IsVec
		: std::false_type
	{
	};

	template <typename T>
	struct IsVec<natVec2<T>>
		: std::true_type
	{
	};

	template <typename T>
	struct IsVec<natVec3<T>>
		: std::true_type
	{
	};

	template <typename T>
	struct IsVec<natVec4<T>>
		: std::true_type
	{
	};

	///	@}
}
