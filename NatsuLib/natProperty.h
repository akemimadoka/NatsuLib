#pragma once

#include "natDelegate.h"

namespace NatsuLib
{
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

	template <typename T>
	class Property final
	{
	public:
		typedef Delegate<T()> GetterDelegate;
		typedef Delegate<void(T const&)> CopySetterDelegate;
		typedef Delegate<void(T &&)> MoveSetterDelegate;

		explicit Property(GetterDelegate getter = {}, CopySetterDelegate copySetter = {}, MoveSetterDelegate moveSetter = {})
			: m_Getter{ std::move(getter) }, m_CopySetter{ std::move(copySetter) }, m_MoveSetter{ std::move(moveSetter) }
		{
		}

		explicit Property(const T& var)
			: Property([&var]
			{
				return var;
			})
		{
		}

		explicit Property(T& var, AutoPropertyFlags autoPropertyFlags = AutoPropertyFlags::Getter | AutoPropertyFlags::CopySetter | AutoPropertyFlags::MoveSetter)
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

		~Property()
		{
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

	template <typename T>
	Property<T> make_property(T& var, AutoPropertyFlags autoPropertyFlags)
	{
		return { var, autoPropertyFlags };
	}
}
