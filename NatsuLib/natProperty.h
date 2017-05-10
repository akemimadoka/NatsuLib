#pragma once

#include "natDelegate.h"

namespace NatsuLib
{
	///	@brief	指定自动属性访问器的类型
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
	///	@brief	属性
	///	@tparam	T	属性的类型
	////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	class Property final
	{
	public:
		typedef Delegate<T()> GetterDelegate;
		typedef Delegate<void(T const&)> CopySetterDelegate;
		typedef Delegate<void(T &&)> MoveSetterDelegate;

		///	@brief	以指定的操作作为属性的访问方法
		///	@param	getter		获取器
		///	@param	copySetter	复制设置器
		///	@param	moveSetter	移动设置器
		explicit Property(GetterDelegate getter = {}, CopySetterDelegate copySetter = {}, MoveSetterDelegate moveSetter = {})
			: m_Getter{ std::move(getter) }, m_CopySetter{ std::move(copySetter) }, m_MoveSetter{ std::move(moveSetter) }
		{
		}

		///	@brief	自动生成const引用变量的只读访问器
		///	@param	var	要访问的变量
		explicit Property(const T& var)
			: Property([&var]
			{
				return var;
			})
		{
		}

		///	@brief	自动生成变量的访问器
		///	@param	var					要访问的变量
		///	@param	autoPropertyFlags	要生成的自动属性访问器，按位指定
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

	///	@brief	快速创建属性
	///	@param	var					要创建属性的变量
	///	@param	autoPropertyFlags	要生成的自动属性访问器
	template <typename T>
	Property<T> make_property(T& var, AutoPropertyFlags autoPropertyFlags)
	{
		return { var, autoPropertyFlags };
	}
}
