////////////////////////////////////////////////////////////////////////////////
///	@file	natStream.h
///	@brief	NatsuLib流
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include "natRefObj.h"
#include "natMultiThread.h"

////////////////////////////////////////////////////////////////////////////////
///	@brief	寻址参考位置
////////////////////////////////////////////////////////////////////////////////
enum class NatSeek
{
	Beg,	///< @brief	流开头
	Cur,	///< @brief	当前流位置
	End		///< @brief	流结尾
};

////////////////////////////////////////////////////////////////////////////////
///	@brief	流
////////////////////////////////////////////////////////////////////////////////
struct natStream
	: natRefObj
{
	virtual ~natStream() = default;

	///	@brief		获得最近的错误
	virtual NatErr GetLastErr() const = 0;

	///	@brief		流是否可写
	virtual nBool CanWrite() const = 0;

	///	@brief		流是否可读
	virtual nBool CanRead() const = 0;

	///	@brief		流是否可重新分配大小
	virtual nBool CanResize() const = 0;

	///	@brief		流是否可寻址
	virtual nBool CanSeek() const = 0;

	///	@brief		获得流的大小
	virtual nLen GetSize() const = 0;

	///	@brief		重新分配流的大小
	///	@note		重新分配成功以后指针位于流的开头
	virtual nResult SetSize(nLen Size) = 0;

	///	@brief		获得读写指针的位置
	virtual nLen GetPosition() const = 0;

	///	@brief		设置读写指针的位置
	///	@param[in]	Origin	寻址参考位置
	///	@param[in]	Offset	偏移
	virtual nResult SetPosition(NatSeek Origin, nLong Offset) = 0;

	///	@brief		读取字节数据
	///	@param[out]	pData	数据缓冲区
	///	@param[in]	Length	读取的长度
	///	@return		实际读取长度
	virtual nLen ReadBytes(nData pData, nLen Length) = 0;

	///	@brief		写入字节数据
	///	@param[in]	pData	数据缓冲区
	///	@param[in]	Length	写入的长度
	///	@return		实际写入长度
	virtual nLen WriteBytes(ncData pData, nLen Length) = 0;

	///	@brief		刷新流
	///	@note		仅对有缓存机制的流有效且有意义
	virtual void Flush() = 0;

	///	@brief		锁定流
	///	@note		可能造成阻塞
	///	@warning	流在多线程下使用时必须加锁
	virtual void Lock() = 0;

	///	@brief		尝试锁定流
	///	@warning	流在多线程下使用时必须加锁
	virtual nResult TryLock() = 0;

	///	@brief		解锁流
	///	@note		必须在流已经被Lock或者成功TryLock以后使用
	///	@warning	流在多线程下使用时必须加锁
	virtual void Unlock() = 0;
};

class natMemoryStream;

////////////////////////////////////////////////////////////////////////////////
///	@brief	NatsuLib文件流实现
////////////////////////////////////////////////////////////////////////////////
class natFileStream
	: public natRefObjImpl<natStream>
{
public:
	natFileStream(ncTStr lpFilename, nBool bReadable, nBool bWritable);
	~natFileStream();

	NatErr GetLastErr() const override;
	nBool CanWrite() const override;
	nBool CanRead() const override;
	nBool CanResize() const override;
	nBool CanSeek() const override;
	nLen GetSize() const override;
	nResult SetSize(nLen Size) override;
	nLen GetPosition() const override;
	nResult SetPosition(NatSeek Origin, nLong Offset) override;
	nLen ReadBytes(nData pData, nLen Length) override;
	nLen WriteBytes(ncData pData, nLen Length) override;
	void Flush() override;
	void Lock() override;
	nResult TryLock() override;
	void Unlock() override;

	ncTStr GetFilename() const noexcept;
	HANDLE GetUnsafeHandle() const noexcept;
	natRefPointer<natMemoryStream> MapToMemoryStream();

private:
	HANDLE m_hFile, m_hMappedFile = NULL;
	natRefPointer<natMemoryStream> m_pMappedFile;
	nTString m_Filename;
	nBool m_bReadable, m_bWritable;
	natCriticalSection m_Section;

protected:
	NatErr m_LastErr;
};

////////////////////////////////////////////////////////////////////////////////
///	@brief	NatsuLib内存流实现
////////////////////////////////////////////////////////////////////////////////
class natMemoryStream
	: public natRefObjImpl<natStream>
{
public:
	natMemoryStream();
	natMemoryStream(ncData pData, nLen Length, nBool bReadable, nBool bWritable, nBool bResizable);
	~natMemoryStream();

	static natRefPointer<natMemoryStream> CreateFromExternMemory(nData pData, nLen Length, nBool bReadable, nBool bWritable);

	NatErr GetLastErr() const override;
	nBool CanWrite() const override;
	nBool CanRead() const override;
	nBool CanResize() const override;
	nBool CanSeek() const override;
	nLen GetSize() const override;
	nResult SetSize(nLen Size) override;
	nLen GetPosition() const override;
	nResult SetPosition(NatSeek Origin, nLong Offset) override;
	nLen ReadBytes(nData pData, nLen Length) override;
	nLen WriteBytes(ncData pData, nLen Length) override;
	void Flush() override;
	void Lock() override;
	nResult TryLock() override;
	void Unlock() override;

	nData GetInternalBuffer() noexcept;
	ncData GetInternalBuffer() const noexcept;

private:
	nData m_pData;
	nLen m_Length;
	nLen m_CurPos;
	nBool m_bReadable;
	nBool m_bWritable;
	nBool m_bResizable;
	nBool m_bExtern;
	natCriticalSection m_Section;

	NatErr m_LastErr;
};