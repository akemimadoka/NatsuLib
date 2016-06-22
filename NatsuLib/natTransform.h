////////////////////////////////////////////////////////////////////////////////
///	@file	natTransform.h
///	@brief	NatsuLib变换相关
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cmath>
#include "natVec.h"
#include "natMat.h"

////////////////////////////////////////////////////////////////////////////////
///	@addtogroup	NatsuLib数学
///	@brief		NatsuLib数学部分
///	@{

////////////////////////////////////////////////////////////////////////////////
///	@brief	NatsuLib变换
////////////////////////////////////////////////////////////////////////////////
namespace natTransform
{
	///	@brief	角度转弧度
	template <typename T>
	inline T DegtoRad(T num)
	{
		static_assert(std::is_floating_point<T>::value, "T should be an floating point type");
		return num / static_cast<T>(180.0) * static_cast<T>(
#ifdef M_PI
			M_PI
#else
			3.14159265358979323846
#endif
			);
	}

	///	@brief	弧度转角度
	template <typename T>
	inline T RadtoDeg(T num)
	{
		static_assert(std::is_floating_point<T>::value, "T should be an floating point type");
		return num * static_cast<T>(180.0) / static_cast<T>(
#ifdef M_PI
			M_PI
#else
			3.14159265358979323846
#endif
			);
	}

	template <typename T>
	T dot(natVec2<T> const& x, natVec2<T> const& y)
	{
		natVec2<T> tmp(x * y);

		return tmp.x + tmp.y;
	}

	template <typename T>
	T dot(natVec3<T> const& x, natVec3<T> const& y)
	{
		natVec3<T> tmp(x * y);

		return tmp.x + tmp.y + tmp.z;
	}

	template <typename T>
	T dot(natVec4<T> const& x, natVec4<T> const& y)
	{
		natVec4<T> tmp(x * y);

		return tmp.x + tmp.y + tmp.z + tmp.w;
	}

	template <typename T>
	T length(T const& x)
	{
		static_assert(std::is_arithmetic<T>::value, "T should be an arithmetic type or natVec");

		if (std::is_integral<T>::value)
		{
			return abs(x);
		}

		if (std::is_floating_point<T>::value)
		{
			return fabs(x);
		}
	}

	template <typename T, template <typename> class vectype>
	T length(vectype<T> const& v)
	{
		static_assert(std::is_base_of<natVec, vectype<T>>::value, "T should be an arithmetic type or natVec");
		return sqrt(dot(v, v));
	}

	template <typename T>
	T inversesqrt(T const& x)
	{
		static_assert(std::is_arithmetic<T>::value, "T should be an arithmetic type or natVec");
		return static_cast<T>(1) / sqrt(x);
	}

	template <typename T, template <typename> class vectype>
	auto inversesqrt(vectype<T> const& v) -> decltype(v.call(sqrt))
	{
		static_assert(std::is_base_of<natVec, vectype<T>>::value, "T should be an arithmetic type or natVec");
		return static_cast<T>(1) / v.call(sqrt);
	}

	template <template <typename> class vectype>
	vectype<nFloat> inversesqrt(vectype<nFloat> const& v)
	{
		static_assert(std::is_base_of<natVec, vectype<nFloat>>::value, "T should be an arithmetic type or natVec");
		auto tmp(v);
		auto xhalf(tmp * 0.5f);
		auto p = reinterpret_cast<vectype<nuInt>*>(const_cast<vectype<nFloat>*>(&v));
		auto i = vectype<nuInt>(0x5f375a86) - (*p >> vectype<nuInt>(1));
		auto ptmp = reinterpret_cast<vectype<nFloat>*>(&i);
		tmp = *ptmp;
		tmp = tmp * (1.5f - xhalf * tmp * tmp);
		return tmp;
	}

	template <typename T>
	T normalize(T const& x)
	{
		static_assert(std::is_arithmetic<T>::value, "T should be an arithmetic type or natVec");

		return x < T(0) ? T(-1) : T(1);
	}

	template <typename T, template <typename> class vectype>
	vectype<T> normalize(vectype<T> const& v)
	{
		static_assert(std::is_base_of<natVec, vectype<T>>::value, "T should be an arithmetic type or natVec");
		return v * inversesqrt(dot(v, v));
	}

	template <typename T>
	T faceforward(T const& N, T const& I, T const& Nref)
	{
		return dot(Nref, I) < static_cast<T>(0) ? N : -N;
	}

	template <typename T>
	T reflect(T const& I, T const& N)
	{
		return I - N * dot(N, I) * static_cast<T>(2);
	}

	template <typename T>
	T refract(T const& I, T const& N, typename T::type const& eta)
	{
		typename T::type const dotValue(dot(N, I));
		typename T::type const k(static_cast<typename T::type>(1) - eta * eta * (static_cast<typename T::type>(1) - dotValue * dotValue));
		return (eta * I - (eta * dotValue + std::sqrt(k)) * N) * static_cast<typename T::type>(k >= static_cast<typename T::type>(0));
	}

	template <typename T>
	T distance(T const& p0, T const& p1)
	{
		return length(p1 - p0);
	}

	template <typename T>
	natVec3<T> cross(natVec3<T> const& x, natVec3<T> const& y)
	{
		return natVec3<T>(
			x.y * y.z - y.y * x.z,
			x.z * y.x - y.z * x.x,
			x.x * y.y - y.x * x.y);
	}

	template <typename T, typename U>
	T mix(T const& x, T const& y, U const& a)
	{
		static_assert(std::is_floating_point<U>::value, "type of a should be floating-point");

		return static_cast<T>(static_cast<U>(x) + a * static_cast<U>(y - x));
	}

	template <typename T>
	T mix(T const& x, T const& y, nBool a)
	{
		return a ? y : x;
	}

	template <typename T, typename U, template<typename> class vecType>
	vecType<T> mix(vecType<T> const& x, vecType<T> const& y, vecType<U> const& a)
	{
		static_assert(std::is_floating_point<U>::value, "type of a should be floating-point");

		return vecType<T>(vecType<U>(x) + a * vecType<U>(y - x));
	}

	template <typename T, template<typename> class vecType>
	vecType<T> mix(vecType<T> const& x, vecType<T> const& y, vecType<nBool> const& a)
	{
		vecType<T> Result;
		for (nuInt i = 0u; i < x.length(); ++i)
		{
			Result[i] = a[i] ? y[i] : x[i];
		}

		return Result;
	}

	template <typename T, typename U, template<typename> class vecType>
	vecType<T> mix(vecType<T> const& x, vecType<T> const& y, U const& a)
	{
		static_assert(std::is_floating_point<U>::value, "type of a should be floating-point");

		return vecType<T>(vecType<U>(x) + a * vecType<U>(y - x));
	}

	template <typename T, template<typename> class vecType>
	vecType<T> mix(vecType<T> const& x, vecType<T> const& y, nBool a)
	{
		return a ? y : x;
	}

	template <typename T>
	natMat4<T> move(natMat4<T> m, natVec3<T> const& v)
	{
		auto a = &m[0][0];
		a[0] += v.x * a[3];
		a[1] += v.y * a[3];
		a[2] += v.z * a[3];
		a[4] += v.x * a[7];
		a[5] += v.y * a[7];
		a[6] += v.z * a[7];
		a[8] += v.x * a[11];
		a[9] += v.y * a[11];
		a[10] += v.z * a[11];
		a[12] += v.x * a[15];
		a[13] += v.y * a[15];
		a[14] += v.z * a[15];

		return m;
	}

	template <typename T>
	natMat4<T> translate(natMat4<T> m, natVec3<T> const& v)
	{
		m[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
		return m;
	}

	template <typename T>
	natMat4<T> rotate(natMat4<T> const& m, T const& angle, natVec3<T> const& v)
	{
		T const c = cos(angle);
		T const s = sin(angle);

		natVec3<T> axis(normalize(v));
		natVec3<T> temp((T(1) - c) * axis);

		natMat4<T> Rotate;
		Rotate[0][0] = c + temp[0] * axis[0];
		Rotate[0][1] = 0 + temp[0] * axis[1] + s * axis[2];
		Rotate[0][2] = 0 + temp[0] * axis[2] - s * axis[1];

		Rotate[1][0] = 0 + temp[1] * axis[0] - s * axis[2];
		Rotate[1][1] = c + temp[1] * axis[1];
		Rotate[1][2] = 0 + temp[1] * axis[2] + s * axis[0];

		Rotate[2][0] = 0 + temp[2] * axis[0] + s * axis[1];
		Rotate[2][1] = 0 + temp[2] * axis[1] - s * axis[0];
		Rotate[2][2] = c + temp[2] * axis[2];

		natMat4<T> Result;
		Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
		Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
		Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
		Result[3] = m[3];
		return Result;
	}

	template <typename T>
	natMat4<T> scale(natMat4<T> const& m, natVec3<T> const& v)
	{
		return natMat4<T>(
			m[0] * v[0],
			m[1] * v[1],
			m[2] * v[2],
			m[3]);
	}

	template <typename T>
	natMat4<T> scale_slow(natMat4<T> const& m, natVec3<T> const& v)
	{
		return m * natMat4<T>(v.x, v.y, v.z, static_cast<T>(1));
	}

	template <typename T>
	natMat4<T> ortho(
		T const& left,
		T const& right,
		T const& bottom,
		T const& top,
		T const& zNear = static_cast<T>(-1),
		T const& zFar = static_cast<T>(1))
	{
		return natMat4<T>(
			static_cast<T>(2) / (right - left), 0, 0, 0,
			0, static_cast<T>(2) / (top - bottom), 0, 0,
			0, 0, -static_cast<T>(2) / (zFar - zNear), 0,
			-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(zFar + zNear) / (zFar - zNear), 1);
	}

	template <typename T>
	natMat4<T> frustum(
		T const& left,
		T const& right,
		T const& bottom,
		T const& top,
		T const& nearVal,
		T const& farVal)
	{
		return natMat4<T>(
			(static_cast<T>(2) * nearVal) / (right - left), 0, 0, 0,
			0, (static_cast<T>(2) * nearVal) / (top - bottom), 0, 0,
			(right + left) / (right - left), (top + bottom) / (top - bottom), -(farVal + nearVal) / (farVal - nearVal), static_cast<T>(-1),
			0, 0, -(static_cast<T>(2) * farVal * nearVal) / (farVal - nearVal), 0);
	}

	template <typename T>
	natMat4<T> perspective(
		T const& fovy,
		T const& aspect,
		T const& zNear,
		T const& zFar)
	{
		if (abs(aspect - std::numeric_limits<T>::epsilon()) <= static_cast<T>(0))
		{
			nat_Throw(natException, _T("aspect too small"));
		}

		T const tanHalfFovy = tan(fovy / static_cast<T>(2));

		return natMat4<T>(
			static_cast<T>(1) / (aspect * tanHalfFovy), 0, 0, 0,
			0, static_cast<T>(1) / (tanHalfFovy), 0, 0,
			0, 0, -(zFar + zNear) / (zFar - zNear), -static_cast<T>(1),
			0, 0, -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear), 0);
	}

	template <typename T>
	natMat4<T> perspective(
		T const& fovy,
		T const& aspect,
		T const& zNear)
	{
		if (abs(aspect - std::numeric_limits<T>::epsilon()) <= static_cast<T>(0))
		{
			nat_Throw(natException, _T("aspect too small"));
		}

		T const tanHalfFovy = tan(fovy / static_cast<T>(2));

		return natMat4<T>(
			static_cast<T>(1) / (aspect * tanHalfFovy), 0, 0, 0,
			0, static_cast<T>(1) / (tanHalfFovy), 0, 0,
			0, 0, -static_cast<T>(1), -static_cast<T>(1),
			0, 0, -static_cast<T>(2) * zNear, 0);
	}

	template <typename T>
	natVec3<T> project(
		natVec3<T> const& obj,
		natMat4<T> const& model,
		natMat4<T> const& proj,
		natVec4<T> const& viewport
		)
	{
		natVec4<T> tmp(obj.x, obj.y, obj.z, static_cast<T>(1));
		tmp = model * tmp;
		tmp = proj * tmp;

		tmp /= tmp.w;
		tmp = tmp * T(0.5) + T(0.5);
		tmp[0] = tmp[0] * T(viewport[2]) + T(viewport[0]);
		tmp[1] = tmp[1] * T(viewport[3]) + T(viewport[1]);

		return natVec3<T>(tmp.x, tmp.y, tmp.z);
	}

	template <typename T>
	natVec3<T> unProject(
		natVec3<T> const& win,
		natMat4<T> const& model,
		natMat4<T> const& proj,
		natVec4<T> const& viewport
		)
	{
		natVec4<T> tmp(win.x, win.y, win.z, T(1));
		tmp.x = (tmp.x - T(viewport[0])) / T(viewport[2]);
		tmp.y = (tmp.y - T(viewport[1])) / T(viewport[3]);
		tmp = tmp * T(2) - T(1);

		natVec4<T> obj = (proj * model).inverse() * tmp;
		obj /= obj.w;

		return natVec3<T>(obj);
	}

	template <typename T>
	natMat4<T> lookAt(
		natVec3<T> const& eye,
		natVec3<T> const& center,
		natVec3<T> const& up
		)
	{
		natVec3<T> const f(normalize(center - eye));
		natVec3<T> const s(normalize(cross(f, up)));
		natVec3<T> const u(cross(s, f));

		return natMat4<T>(
			s.x, u.x, -f.x, 0,
			s.y, u.y, -f.y, 0,
			s.z, u.z, -f.z, 0,
			-dot(s, eye), -dot(u, eye), dot(f, eye), 1);
	}
}

/// @}