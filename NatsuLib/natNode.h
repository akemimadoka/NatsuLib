#pragma once

#pragma warning(push)
#pragma warning(disable : 4180)
#include <functional>
#pragma warning(pop)

#include "natRefObj.h"

struct natNode
	: natRefObj
{
	virtual void AddChild(natNode* pChild) = 0;
	virtual nBool ChildExists(natNode* pChild) const noexcept = 0;
	virtual nBool ChildExists(ncTStr Name) const noexcept = 0;
	virtual natNode* GetChildByName(ncTStr Name) const = 0;

	template <typename T>
	T* GetChildByName(ncTStr Name) const
	{
		return dynamic_cast<T*>(GetChildByName(Name));
	}

	/**
	* @brief 枚举子节点
	* @param[in] Recursive 递归地枚举
	* @param[in] EnumCallback 枚举回调函数，接受一个参数，其类型为natNode*，表示此次枚举到的节点，返回值为bool，返回true时枚举立即终止
	* @return 是否因为EnumCallback返回true而终止枚举
	*/
	virtual nBool EnumChildNode(nBool Recursive, std::function<nBool(natNode*)> EnumCallback) = 0;

	virtual size_t GetChildCount() const noexcept = 0;
	virtual void SetParent(natNode* pParent) = 0;
	virtual natNode* GetParent() const noexcept = 0;
	virtual void RemoveChild(natNode* pnatNode) = 0;
	virtual void RemoveChildByName(ncTStr Name) = 0;
	virtual void RemoveAllChild() = 0;

	virtual void SetName(ncTStr Name) = 0;
	virtual ncTStr GetName() const noexcept = 0;
};