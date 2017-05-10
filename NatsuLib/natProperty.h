#pragma once

#include "natDelegate.h"

namespace NatsuLib
{
	///	@brief	ָ���Զ����Է�����������
	enum class AutoPropertyFlags : nuInt
	{
		None = 0,

		Getter = 1,
		CopySetter = 2,
		MoveSetter = 4,

		AllSetter = CopySetter | MoveSetter,
		All = Getter | AllSetter,
	};

	MAKE_ENUM_CLASS_BITMASK_TYPE(AutoPropertyFlags);

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	����
	///	@tparam	T	���Ե�����
	////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	class Property final
	{
	public:
		typedef Delegate<T()> GetterDelegate;
		typedef Delegate<void(T const&)> CopySetterDelegate;
		typedef Delegate<void(T &&)> MoveSetterDelegate;

		///	@brief	��ָ���Ĳ�����Ϊ���Եķ��ʷ���
		///	@param	getter		��ȡ��
		///	@param	copySetter	����������
		///	@param	moveSetter	�ƶ�������
		explicit Property(GetterDelegate getter = {}, CopySetterDelegate copySetter = {}, MoveSetterDelegate moveSetter = {})
			: m_Getter{ std::move(getter) }, m_CopySetter{ std::move(copySetter) }, m_MoveSetter{ std::move(moveSetter) }
		{
		}

		///	@brief	�Զ�����const���ñ�����ֻ��������
		///	@param	var	Ҫ���ʵı���
		explicit Property(const T& var)
			: Property([&var]
			{
				return var;
			})
		{
		}

		///	@brief	�Զ����ɱ����ķ�����
		///	@param	var					Ҫ���ʵı���
		///	@param	autoPropertyFlags	Ҫ���ɵ��Զ����Է���������λָ��
		explicit Property(T& var, AutoPropertyFlags autoPropertyFlags = AutoPropertyFlags::All)
		{
			if ((autoPropertyFlags & AutoPropertyFlags::Getter) != AutoPropertyFlags::None)
			{
				m_Getter = [&var]
				{
					return var;
				};
			}
			if ((autoPropertyFlags & AutoPropertyFlags::CopySetter) != AutoPropertyFlags::None)
			{
				m_CopySetter = [&var](T const& value)
				{
					var = value;
				};
			}
			if ((autoPropertyFlags & AutoPropertyFlags::MoveSetter) != AutoPropertyFlags::None)
			{
				m_MoveSetter = [&var](T && value)
				{
					var = std::move(value);
				};
			}
		}

		operator T() const
		{
			if (!m_Getter)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This property does not support getter."_nv);
			}
			return m_Getter();
		}

		Property& operator=(T const& other)
		{
			if (!m_CopySetter)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This property does not support copy setter."_nv);
			}
			m_CopySetter(other);
			return *this;
		}

		Property& operator=(T && other)
		{
			if (!m_MoveSetter)
			{
				return *this = static_cast<T const&>(other);
			}
			m_MoveSetter(std::move(other));
			return *this;
		}

	private:
		GetterDelegate m_Getter;
		CopySetterDelegate m_CopySetter;
		MoveSetterDelegate m_MoveSetter;
	};

	///	@brief	���ٴ�������
	///	@param	var					Ҫ�������Եı���
	///	@param	autoPropertyFlags	Ҫ���ɵ��Զ����Է�����
	template <typename T>
	Property<T> make_property(T& var, AutoPropertyFlags autoPropertyFlags)
	{
		return { var, autoPropertyFlags };
	}
}
