////////////////////////////////////////////////////////////////////////////////
///	@file	natStream.h
///	@brief	NatsuLib流
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "natRefObj.h"
#include "natMultiThread.h"
#include "natString.h"
#include "natException.h"

#ifndef _WIN32
#	include <fstream>
#endif

namespace NatsuLib
{
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
		virtual ~natStream();

		///	@brief		流是否可写
		virtual nBool CanWrite() const = 0;

		///	@brief		流是否可读
		virtual nBool CanRead() const = 0;

		///	@brief		流是否可重新分配大小
		virtual nBool CanResize() const = 0;

		///	@brief		流是否可寻址
		virtual nBool CanSeek() const = 0;

		///	@brief		流是否已到达结尾
		virtual nBool IsEndOfStream() const = 0;

		///	@brief		获得流的大小
		virtual nLen GetSize() const = 0;

		///	@brief		重新分配流的大小
		///	@note		重新分配成功以后指针位于流的开头
		virtual void SetSize(nLen Size) = 0;

		///	@brief		获得读写指针的位置
		virtual nLen GetPosition() const = 0;

		///	@brief		设置读写指针的位置
		///	@param[in]	Origin	寻址参考位置
		///	@param[in]	Offset	偏移
		virtual void SetPosition(NatSeek Origin, nLong Offset) = 0;

		/// @brief		从流中读取一个字节
		virtual nByte ReadByte();

		///	@brief		读取字节数据
		///	@param[out]	pData	数据缓冲区
		///	@param[in]	Length	读取的长度
		///	@return		实际读取长度
		virtual nLen ReadBytes(nData pData, nLen Length) = 0;

		///	@brief		强制读取字节数据
		///	@param[out]	pData	数据缓冲区
		///	@param[in]	Length	读取的长度
		///	@note		重复读取操作直到成功读取的字节数不小于Length为止，注意本方法可能造成死循环
		virtual void ForceReadBytes(nData pData, nLen Length);

		/// @brief		异步读取字节数据
		/// @param[out]	pData	数据缓冲区
		/// @param[in]	Length	读取的长度
		/// @return		实际读取长度
		virtual std::future<nLen> ReadBytesAsync(nData pData, nLen Length);

		/// @brief		向流中写入一个字节
		virtual void WriteByte(nByte byte);

		///	@brief		写入字节数据
		///	@param[in]	pData	数据缓冲区
		///	@param[in]	Length	写入的长度
		///	@return		实际写入长度
		virtual nLen WriteBytes(ncData pData, nLen Length) = 0;

		///	@brief		强制写入字节数据
		///	@param[in]	pData	数据缓冲区
		///	@param[in]	Length	写入的长度
		///	@note		重复写入操作直到成功写入的字节数不小于Length为止，注意本方法可能造成死循环
		virtual void ForceWriteBytes(ncData pData, nLen Length);

		///	@brief		异步写入字节数据
		///	@param[in]	pData	数据缓冲区
		///	@param[in]	Length	写入的长度
		///	@return		实际写入长度
		virtual std::future<nLen> WriteBytesAsync(ncData pData, nLen Length);

		///	@brief		刷新流
		///	@note		仅对有缓存机制的流有效且有意义
		virtual void Flush() = 0;
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
		static natRefPointer<natMemoryStream> CreateFromExternMemory(ncData pData, nLen Length, nBool bReadable);

		nBool CanWrite() const override;
		nBool CanRead() const override;
		nBool CanResize() const override;
		nBool CanSeek() const override;
		nBool IsEndOfStream() const override;
		nLen GetSize() const override;
		void SetSize(nLen Size) override;
		nLen GetPosition() const override;
		void SetPosition(NatSeek Origin, nLong Offset) override;
		nByte ReadByte() override;
		nLen ReadBytes(nData pData, nLen Length) override;
		std::future<nLen> ReadBytesAsync(nData pData, nLen Length) override;
		void WriteByte(nByte byte) override;
		nLen WriteBytes(ncData pData, nLen Length) override;
		std::future<nLen> WriteBytesAsync(ncData pData, nLen Length) override;
		void Flush() override;

		nData GetInternalBuffer() noexcept;
		ncData GetInternalBuffer() const noexcept;

	private:
		natCriticalSection m_CriSection;

		nData m_pData;
		nLen m_Length;
		nLen m_CurPos;
		nBool m_bReadable;
		nBool m_bWritable;
		nBool m_bResizable;
		nBool m_bExtern;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	NatsuLib文件流实现
	////////////////////////////////////////////////////////////////////////////////
	class natFileStream
		: public natRefObjImpl<natStream>
	{
	public:
#ifdef _WIN32
		typedef HANDLE UnsafeHandle;
#endif

#ifdef _WIN32
		natFileStream(nStrView filename, nBool bReadable, nBool bWritable, nBool isAsync = false);
		natFileStream(UnsafeHandle hFile, nBool bReadable, nBool bWritable, nBool transferOwner = false, nBool isAsync = false);
#else
		natFileStream(nStrView filename, nBool bReadable, nBool bWritable);
#endif

		~natFileStream();

		nBool CanWrite() const override;
		nBool CanRead() const override;
		nBool CanResize() const override;
		nBool CanSeek() const override;
		nBool IsEndOfStream() const override;
		nLen GetSize() const override;
		void SetSize(nLen Size) override;
		nLen GetPosition() const override;
		void SetPosition(NatSeek Origin, nLong Offset) override;
		nByte ReadByte() override;
		nLen ReadBytes(nData pData, nLen Length) override;
		void WriteByte(nByte byte) override;
		nLen WriteBytes(ncData pData, nLen Length) override;
#ifdef _WIN32
		std::future<nLen> ReadBytesAsync(nData pData, nLen Length) override;
		std::future<nLen> WriteBytesAsync(ncData pData, nLen Length) override;
#endif

		void Flush() override;

		nStrView GetFilename() const noexcept;

#ifdef _WIN32
		UnsafeHandle GetUnsafeHandle() const noexcept;
		natRefPointer<natMemoryStream> MapToMemoryStream();
#endif

	private:
#ifdef _WIN32
		UnsafeHandle m_hFile, m_hMappedFile;
		natRefPointer<natMemoryStream> m_pMappedFile;
		const nBool m_ShouldDispose;
		const nBool m_IsAsync;
#else
		std::fstream m_File;
		nLen m_Size, m_CurrentPos;
#endif
		
		nString m_Filename;
		nBool m_bReadable, m_bWritable;
	};

	class natSubStream
		: public natRefObjImpl<natStream>
	{
	public:
		natSubStream(natRefPointer<natStream> stream, nLen startPosition, nLen endPosition);
		~natSubStream();

		natRefPointer<natStream> GetUnderlyingStream() const noexcept;

		nBool CanWrite() const override;
		nBool CanRead() const override;
		nBool CanResize() const override;
		nBool CanSeek() const override;
		nBool IsEndOfStream() const override;
		nLen GetSize() const override;
		void SetSize(nLen Size) override;
		nLen GetPosition() const override;
		void SetPosition(NatSeek Origin, nLong Offset) override;
		nByte ReadByte() override;
		nLen ReadBytes(nData pData, nLen Length) override;
		std::future<nLen> ReadBytesAsync(nData pData, nLen Length) override;
		void WriteByte(nByte byte) override;
		nLen WriteBytes(ncData pData, nLen Length) override;
		std::future<nLen> WriteBytesAsync(ncData pData, nLen Length) override;
		void Flush() override;

	private:
		const natRefPointer<natStream> m_InternalStream;
		const nLen m_StartPosition;
		const nLen m_EndPosition;
		nLen m_CurrentPosition;

		void adjustPosition() const;
		void checkPosition() const;
	};

	class natStdStream
		: public natRefObjImpl<natStream>
	{
	public:
		enum StdStreamType
		{
			StdIn,
			StdOut,
			StdErr,
		};

#ifdef _WIN32
		typedef HANDLE NativeHandle;

		nBool UseFileApi() const noexcept;
#else
		typedef FILE* NativeHandle;
#endif

		explicit natStdStream(StdStreamType stdStreamType);
		~natStdStream();

		nBool CanWrite() const override;
		nBool CanRead() const override;
		nBool CanResize() const override;
		nBool CanSeek() const override;
		nBool IsEndOfStream() const override;
		nLen GetSize() const override;
		void SetSize(nLen /*Size*/) override;
		nLen GetPosition() const override;
		void SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override;
		nByte ReadByte() override;
		nLen ReadBytes(nData pData, nLen Length) override;
		std::future<nLen> ReadBytesAsync(nData pData, nLen Length) override;
		void WriteByte(nByte byte) override;
		nLen WriteBytes(ncData pData, nLen Length) override;
		std::future<nLen> WriteBytesAsync(ncData pData, nLen Length) override;
		void Flush() override;

	private:
		StdStreamType m_StdStreamType;
		NativeHandle m_StdHandle;
#ifdef _WIN32
		natRefPointer<natFileStream> m_InternalStream;
#endif
	};
}
