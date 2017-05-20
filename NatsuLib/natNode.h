#pragma once

#include "natRefObj.h"
#include <functional>

namespace NatsuLib
{
	struct natNode
		: natRefObj
	{
		virtual void AddChild(natRefPointer<natNode> pChild) = 0;
		virtual nBool ChildExists(natRefPointer<natNode> pChild) const noexcept = 0;
		virtual nBool ChildExists(nStrView Name) const noexcept = 0;
		virtual natRefPointer<natNode> GetChildByName(nStrView Name) const = 0;

		template <typename T>
		natRefPointer<T> GetChildByName(nStrView Name) const
		{
			return static_cast<natRefPointer<T>>(GetChildByName(Name));
		}

		/**
		* @brief 枚举子节点
		* @param[in] Recursive 递归地枚举
		* @param[in] EnumCallback 枚举回调函数，接受一个参数，其类型为natNode*，表示此次枚举到的节点，返回值为bool，返回true时枚举立即终止
		* @return 是否因为EnumCallback返回true而终止枚举
		*/
		virtual nBool EnumChildNode(nBool Recursive, std::function<nBool(natRefPointer<natNode>)> EnumCallback) = 0;

		virtual size_t GetChildCount() const noexcept = 0;
		virtual void SetParent(natNode* pParent) = 0;
		virtual natRefPointer<natNode> GetParent() const noexcept = 0;
		virtual void RemoveChild(natRefPointer<natNode> pnatNode) = 0;
		virtual void RemoveChildByName(nStrView Name) = 0;
		virtual void RemoveAllChild() = 0;

		virtual void SetName(nStrView Name) = 0;
		virtual nStrView GetName() const noexcept = 0;
	};
}
