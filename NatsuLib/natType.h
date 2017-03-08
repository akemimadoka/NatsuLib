////////////////////////////////////////////////////////////////////////////////
///	@file	natType.h
///	@brief	描述NatsuLib中基本数据类型、部分宏及内联函数
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "natConfig.h"
#include <cstdint>
#include <type_traits>

#define NVIMPL(text) text##_nv
#define NV(text) NVIMPL(text)

////////////////////////////////////////////////////////////////////////////////
///	@addtogroup	Natsu库基本数据类型
///	@brief		Natsu库基本数据类型定义
///	@{

typedef	bool				nBool;		///< @brief	逻辑型
typedef char				nChar;		///< @brief	字符型
typedef	wchar_t				nWChar;		///< @brief	宽字符型
typedef int8_t				nSByte;		///< @brief	有符号字节型
typedef	uint8_t				nByte;		///< @brief	字节型
typedef int16_t				nShort;		///< @brief	16位短整数
typedef	uint16_t			nuShort;	///< @brief	16位无符号短整数
typedef	int32_t				nInt;		///< @brief	32位整数
typedef	uint32_t			nuInt;		///< @brief	32位无符号整数
typedef int64_t				nLong;		///< @brief	64位长整数
typedef	uint64_t			nuLong;		///< @brief	64位无符号长整数
typedef	float				nFloat;		///< @brief	单精度浮点数
typedef	double				nDouble;	///< @brief	双精度浮点数
typedef	nChar*				nStr;		///< @brief	C风格字符串
typedef	const nChar*		ncStr;		///< @brief	C风格常量字符串
typedef	nWChar*				nWStr;		///< @brief	C风格宽字符串
typedef	const nWChar*		ncWStr;		///< @brief	C风格常量宽字符串
typedef	nByte*				nData;		///< @brief	内存数据指针
typedef	const nByte*		ncData;		///< @brief	常量内存数据指针
typedef	nuLong				nLen;		///< @brief	长度型
typedef	nInt				nResult;	///< @brief	预定义返回值
										///< @details
										///	nResult 定义	：\n
										///	符  号  位	： 0 - 成功 1 - 失败\n
										///	30 - 16 位	： 保留\n
										///	15 - 0  位	： 错误描述

template <typename T>
using nUnsafePtr = std::add_pointer_t<T>;

///	@}

////////////////////////////////////////////////////////////////////////////////
///	@addtogroup	Natsu库基本宏
///	@brief		Natsu库基本宏定义
///	@{

#ifdef _MSC_VER
///	@brief	强制内联
#define	NATINLINE			__forceinline
///	@brief	拒绝内联
#define NATNOINLINE			__declspec(noinline)
#else
///	@brief	强制内联
#define	NATINLINE			inline
///	@brief	拒绝内联
#define NATNOINLINE
#endif

///	@brief	是否成功
#define NATOK(x)			(((nResult)(x)) >= 0)
///	@brief	是否失败
#define	NATFAIL(x)			(((nResult)(x)) <  0)
///	@brief	创建错误码
#define	NATMAKEERR(code)	((nResult) (0x80000000 | ((nInt)(code))))
///	@brief	状态码
#define	NATRESULTCODE(code)	((code) & 0xFFFF)

///	@}

////////////////////////////////////////////////////////////////////////////////
///	@addtogroup	Natsu库常见错误
///	@brief		常见错误
///	@{

enum /*[[nodiscard]]*/ NatErr : nResult
{
	NatErr_Interrupted	=	1,		///< @brief	正常中断

	NatErr_OK			=	0,		///< @brief	正常

	NatErr_Unknown		=	NATMAKEERR(1),	///< @brief	未知错误
	NatErr_IllegalState	=	NATMAKEERR(2),	///< @brief	非法状态
	NatErr_InvalidArg	=	NATMAKEERR(3),	///< @brief	无效参数
	NatErr_InternalErr	=	NATMAKEERR(4),	///< @brief	内部错误
	NatErr_OutOfRange	=	NATMAKEERR(5),	///< @brief	超出范围
	NatErr_NotImpl		=	NATMAKEERR(6),	///< @brief	未实现
	NatErr_NotSupport	=	NATMAKEERR(7),	///< @brief	不支持的功能
	NatErr_Duplicated	=	NATMAKEERR(8),	///< @brief	发生重复
	NatErr_NotFound		=	NATMAKEERR(9),	///< @brief	未找到
};

///	@}

////////////////////////////////////////////////////////////////////////////////
///	@addtogroup	Natsu库基本内联函数
///	@brief		Natsu库基本内联函数定义
///	@{

///	@brief		安全删除指针
///	@warning	避免使用本函数
template <typename T>
NATINLINE void SafeDel(T* volatile & ptr)
{
	delete ptr;
	ptr = nullptr;
}

///	@brief		安全删除数组
///	@warning	避免使用本函数
template <typename T>
NATINLINE void SafeDelArr(T* volatile & ptr)
{
	delete[] ptr;
	ptr = nullptr;
}

namespace NatsuLib
{
	namespace detail_
	{
		template <typename T, typename Enable = void>
		struct CanAddRef
			: std::false_type
		{
		};

		template <typename T>
		struct CanAddRef<T, std::void_t<decltype(std::declval<const volatile T>().AddRef())>>
			: std::true_type
		{
		};

		template <typename T, typename Enable = void>
		struct CanRelease
			: std::false_type
		{
		};

		template <typename T>
		struct CanRelease<T, std::void_t<decltype(std::declval<const volatile T>().Release())>>
			: std::true_type
		{
		};
	}
}

///	@brief	安全释放
template <typename T>
NATINLINE std::enable_if_t<NatsuLib::detail_::CanRelease<T>::value> SafeRelease(T* volatile & ptr)
{
	const auto ptrToRelease = ptr;
	if (ptrToRelease != nullptr)
	{
		ptrToRelease->Release();
		if (ptr == ptrToRelease)
		{
			ptr = nullptr;	// 可能非线程安全？
		}
	}
}

///	@}
