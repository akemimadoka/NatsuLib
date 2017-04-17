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
		enum
		{
			DefaultCopyToBufferSize = 1024,
		};

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

		///	@brief		将流中的内容复制到另一流
		///	@param[in]	other	要复制到的流
		///	@return		总实际读取长度
		///	@note		读取长度不意味着成功写入到另一流的长度
		virtual nLen CopyTo(natRefPointer<natStream> const& other);

		///	@brief		刷新流
		///	@note		仅对有缓存机制的流有效且有意义
		virtual void Flush() = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	包装流
	///	@note	包装流具有一个内部流，默认所有操作直接转发到内部流\n
	///			可通过继承此类完成对数据的特殊处理\n
	///			注意析构时本类的析构函数会先执行，之后才会析构内部流
	////////////////////////////////////////////////////////////////////////////////
	class natWrappedStream
		: public natRefObjImpl<natWrappedStream, natStream>
	{
	public:
		explicit natWrappedStream(natRefPointer<natStream> stream);
		~natWrappedStream();

		///	@brief	获得内部流
		virtual natRefPointer<natStream> GetUnderlyingStream() const noexcept;

		///	@brief	连续枚举内部流，直到内部流不是natWrappedStream或者enumerator返回true为止
		///	@param	enumerator	枚举函数，返回true会立即停止枚举
		///	@note	存在形成环的可能，此时本方法可能会进入死循环
		///	@return	枚举是否由于enumerator返回true而中止
		nBool EnumUnderlyingStream(std::function<bool(natWrappedStream&)> const& enumerator) const;

		///	@brief	连续获得内部流，如果内部流的类型是T则返回它，如果无法获得这个类型的内部流则返回nullptr
		///	@tparam	T	要获得的内部流的类型
		template <typename T>
		std::enable_if_t<std::is_base_of<natStream, T>::value, natRefPointer<T>> GetUnderlyingStreamAs() const noexcept
		{
			return GetUnderlyingStreamAs(typeid(T));
		}

		///	@see GetUnderlyingStreamAs{T}
		natRefPointer<natStream> GetUnderlyingStreamAs(std::type_info const& typeinfo) const noexcept;

		///	@brief	连续获得内部的流，如果流不是natWrappedStream则返回它
		natRefPointer<natStream> GetUltimateUnderlyingStream() const noexcept;

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
		void ForceReadBytes(nData pData, nLen Length) override;
		std::future<nLen> ReadBytesAsync(nData pData, nLen Length) override;
		void WriteByte(nByte byte) override;
		nLen WriteBytes(ncData pData, nLen Length) override;
		void ForceWriteBytes(ncData pData, nLen Length) override;
		std::future<nLen> WriteBytesAsync(ncData pData, nLen Length) override;
		nLen CopyTo(natRefPointer<natStream> const& other) override;
		void Flush() override;

	protected:
		natRefPointer<natStream> m_InternalStream;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	NatsuLib内存流实现
	////////////////////////////////////////////////////////////////////////////////
	class natMemoryStream
		: public natRefObjImpl<natMemoryStream, natStream>
	{
	public:
		natMemoryStream(ncData pData, nLen Length, nBool bReadable, nBool bWritable, nBool autoResize);
		natMemoryStream(nLen Length, nBool bReadable, nBool bWritable, nBool autoResize);
		natMemoryStream(natMemoryStream const& other);
		natMemoryStream(natMemoryStream && other) noexcept;
		~natMemoryStream();

		natMemoryStream& operator=(natMemoryStream const& other);
		natMemoryStream& operator=(natMemoryStream && other) noexcept;

		nBool CanWrite() const override;
		nBool CanRead() const override;
		nBool CanResize() const override;
		nBool CanSeek() const override;
		nBool IsEndOfStream() const override;

		///	@brief	获得实际流内数据大小，并非总容量
		nLen GetSize() const override;

		///	@brief	设置流的大小
		///	@note	无法通过设置较小的大小来截断数据，通过SetSize只能增长容量而无法缩小
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

		void Reserve(nLen newCapacity);
		nLen GetCapacity() const noexcept;

		void ClearAndResetSize(nLen capacity);

	private:
		mutable natCriticalSection m_CriSection;

		nData m_pData;
		nLen m_Size;
		nLen m_Capacity;
		nLen m_CurPos;
		nBool m_bReadable;
		nBool m_bWritable;
		nBool m_AutoResize;

		void allocateAndInvalidateOldData(nLen newCapacity);
	};

	class natExternMemoryStream
		: public natRefObjImpl<natExternMemoryStream, natStream>
	{
	public:
		template <size_t N>
		natExternMemoryStream(nByte(&array)[N], nBool readable, nBool writable)
			: natExternMemoryStream(array, N, readable, writable)
		{
		}

		template <size_t N>
		natExternMemoryStream(const nByte(&array)[N], nBool readable)
			: natExternMemoryStream(array, N, readable)
		{
		}

		natExternMemoryStream(nData externData, nLen size, nBool readable, nBool writable);
		natExternMemoryStream(ncData externData, nLen size, nBool readable);
		~natExternMemoryStream();

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
		void Flush() override;

		ncData GetExternData() const noexcept;

	private:
		const nData m_ExternData;
		const nLen m_Size;
		nLen m_CurrentPos;
		const nBool m_Readable;
		const nBool m_Writable;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	NatsuLib文件流实现
	////////////////////////////////////////////////////////////////////////////////
	class natFileStream
		: public natRefObjImpl<natFileStream, natStream>, public nonmovable
	{
	public:
#ifdef _WIN32
		typedef HANDLE UnsafeHandle;
#else
		typedef int UnsafeHandle;
#endif

#ifdef _WIN32
		natFileStream(nStrView filename, nBool bReadable, nBool bWritable, nBool isAsync = false, nBool truncate = false);
		natFileStream(UnsafeHandle hFile, nBool bReadable, nBool bWritable, nBool transferOwner = false, nBool isAsync = false);
#else
		natFileStream(nStrView filename, nBool bReadable, nBool bWritable, nBool truncate = false);
		natFileStream(UnsafeHandle hFile, nBool bReadable, nBool bWritable, nBool transferOwner = false);
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
		UnsafeHandle GetUnsafeHandle() const noexcept;

#ifdef _WIN32
		natRefPointer<natMemoryStream> MapToMemoryStream();
#endif

	private:
        UnsafeHandle m_hFile;
        const nBool m_ShouldDispose;

#ifdef _WIN32
		UnsafeHandle m_hMappedFile;
		natRefPointer<natMemoryStream> m_pMappedFile;
		const nBool m_IsAsync;
#else
		nBool m_IsEndOfFile;
#endif

		nString m_Filename;
		nBool m_bReadable, m_bWritable;
	};

	class natSubStream
		: public natRefObjImpl<natSubStream, natWrappedStream>
	{
	public:
		natSubStream(natRefPointer<natStream> stream, nLen startPosition, nLen endPosition);
		~natSubStream();

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

	private:
		const nLen m_StartPosition;
		const nLen m_EndPosition;
		nLen m_CurrentPosition;

		void adjustPosition() const;
		void checkPosition() const;
	};

	class natStdStream
		: public natRefObjImpl<natStdStream, natStream>, public nonmovable
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

		NativeHandle GetNativeHandle() const noexcept;

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
