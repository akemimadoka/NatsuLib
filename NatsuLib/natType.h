////////////////////////////////////////////////////////////////////////////////
///	@file	natType.h
///	@brief	����NatsuLib�л����������͡����ֺ꼰��������
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "natConfig.h"
#include <cstdint>
#include <type_traits>

#define NVIMPL(text) text##_nv
#define NV(text) NVIMPL(text)

////////////////////////////////////////////////////////////////////////////////
///	@addtogroup	Natsu�������������
///	@brief		Natsu������������Ͷ���
///	@{

typedef	bool				nBool;		///< @brief	�߼���
typedef char				nChar;		///< @brief	�ַ���
typedef	wchar_t				nWChar;		///< @brief	���ַ���
typedef std::int8_t				nSByte;		///< @brief	�з����ֽ���
typedef	std::uint8_t				nByte;		///< @brief	�ֽ���
typedef std::int16_t				nShort;		///< @brief	16λ������
typedef	std::uint16_t			nuShort;	///< @brief	16λ�޷��Ŷ�����
typedef	std::int32_t				nInt;		///< @brief	32λ����
typedef	std::uint32_t			nuInt;		///< @brief	32λ�޷�������
typedef std::int64_t				nLong;		///< @brief	64λ������
typedef	std::uint64_t			nuLong;		///< @brief	64λ�޷��ų�����
typedef	float				nFloat;		///< @brief	�����ȸ�����
typedef	double				nDouble;	///< @brief	˫���ȸ�����
typedef	nChar*				nStr;		///< @brief	C����ַ���
typedef	const nChar*		ncStr;		///< @brief	C������ַ���
typedef	nWChar*				nWStr;		///< @brief	C�����ַ���
typedef	const nWChar*		ncWStr;		///< @brief	C��������ַ���
typedef	nByte*				nData;		///< @brief	�ڴ�����ָ��
typedef	const nByte*		ncData;		///< @brief	�����ڴ�����ָ��
typedef	nuLong				nLen;		///< @brief	������
typedef	nInt				nResult;	///< @brief	Ԥ���巵��ֵ
										///< @details
										///	nResult ����	��\n
										///	��  ��  λ	�� 0 - �ɹ� 1 - ʧ��\n
										///	30 - 16 λ	�� ����\n
										///	15 - 0  λ	�� ��������

static_assert(sizeof(nLen) >= sizeof(std::size_t), "");

template <typename T>
using nUnsafePtr = std::add_pointer_t<T>;

///	@}

////////////////////////////////////////////////////////////////////////////////
///	@addtogroup	Natsu�������
///	@brief		Natsu������궨��
///	@{

#ifdef _MSC_VER
///	@brief	ǿ������
#define	NATINLINE			__forceinline
///	@brief	�ܾ�����
#define NATNOINLINE			__declspec(noinline)
#else
///	@brief	ǿ������
#define	NATINLINE			inline
///	@brief	�ܾ�����
#define NATNOINLINE
#endif

///	@brief	�Ƿ�ɹ�
#define NATOK(x)			(((nResult)(x)) >= 0)
///	@brief	�Ƿ�ʧ��
#define	NATFAIL(x)			(((nResult)(x)) <  0)
///	@brief	����������
#define	NATMAKEERR(code)	((nResult) (0x80000000 | ((nInt)(code))))
///	@brief	״̬��
#define	NATRESULTCODE(code)	((code) & 0xFFFF)

///	@}

////////////////////////////////////////////////////////////////////////////////
///	@addtogroup	Natsu�ⳣ������
///	@brief		��������
///	@{

enum /*[[nodiscard]]*/ NatErr : nResult
{
	NatErr_Interrupted	=	1,		///< @brief	�����ж�

	NatErr_OK			=	0,		///< @brief	����

	NatErr_Unknown		=	NATMAKEERR(1),	///< @brief	δ֪����
	NatErr_IllegalState	=	NATMAKEERR(2),	///< @brief	�Ƿ�״̬
	NatErr_InvalidArg	=	NATMAKEERR(3),	///< @brief	��Ч����
	NatErr_InternalErr	=	NATMAKEERR(4),	///< @brief	�ڲ�����
	NatErr_OutOfRange	=	NATMAKEERR(5),	///< @brief	������Χ
	NatErr_NotImpl		=	NATMAKEERR(6),	///< @brief	δʵ��
	NatErr_NotSupport	=	NATMAKEERR(7),	///< @brief	��֧�ֵĹ���
	NatErr_Duplicated	=	NATMAKEERR(8),	///< @brief	�����ظ�
	NatErr_NotFound		=	NATMAKEERR(9),	///< @brief	δ�ҵ�
};

///	@}

////////////////////////////////////////////////////////////////////////////////
///	@addtogroup	Natsu�������������
///	@brief		Natsu�����������������
///	@{

///	@brief		��ȫɾ��ָ��
///	@warning	����ʹ�ñ�����
template <typename T>
NATINLINE void SafeDel(T* volatile & ptr)
{
	delete ptr;
	ptr = nullptr;
}

///	@brief		��ȫɾ������
///	@warning	����ʹ�ñ�����
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

///	@brief	��ȫ�ͷ�
template <typename T>
NATINLINE std::enable_if_t<NatsuLib::detail_::CanRelease<T>::value> SafeRelease(T* volatile & ptr)
{
	const auto ptrToRelease = ptr;
	if (ptrToRelease != nullptr)
	{
		ptrToRelease->Release();
		if (ptr == ptrToRelease)
		{
			ptr = nullptr;	// ���ܷ��̰߳�ȫ��
		}
	}
}

///	@}
