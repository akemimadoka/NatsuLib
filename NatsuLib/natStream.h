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

		///	@brief		从开头设置读写指针的位置
		///	@param[in]	Offset	偏移
		virtual void SetPositionFromBegin(nLen Offset);

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
		///	@note		对于无法反馈已写入字节数的流本方法返回的值无意义
		virtual nLen WriteBytes(ncData pData, nLen Length) = 0;

		///	@brief		强制写入字节数据
		///	@param[in]	pData	数据缓冲区
		///	@param[in]	Length	写入的长度
		///	@note		重复写入操作直到成功写入的字节数不小于Length为止，注意本方法可能造成死循环
		///	@warning	并非对于所有流都适用，对于无法反馈已写入字节数的流禁止使用本方法
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
	///	@note	包装流具有一个内部流，默认除了CopyTo以外的所有操作直接转发到内部流\n
	///			可通过继承此类完成对数据的特殊处理\n
	///			注意析构时本类的析构函数会先执行，之后才会释放内部流
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
		nBool EnumUnderlyingStream(std::function<nBool(natWrappedStream&)> const& enumerator) const;

		///	@brief	连续获得内部流，如果内部流的类型是T则返回它，如果无法获得这个类型的内部流则返回nullptr
		///	@tparam	T	要获得的内部流的类型
		template <typename T>
		std::enable_if_t<std::is_base_of<natStream, T>::value, natRefPointer<T>> GetUnderlyingStreamAs() const noexcept
		{
			return GetUnderlyingStreamAs(typeid(T));
		}

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
		nLen ReadBytes(nData pData, nLen Length) override;
		nLen WriteBytes(ncData pData, nLen Length) override;
		void Flush() override;

	protected:
		natRefPointer<natStream> m_InternalStream;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	释放回调流
	///	@note	用于在释放时进行特殊处理的流，其他操作直接转发到内部流
	////////////////////////////////////////////////////////////////////////////////
	class DisposeCallbackStream final
		: public natRefObjImpl<DisposeCallbackStream, natWrappedStream>
	{
	public:
		explicit DisposeCallbackStream(natRefPointer<natStream> internalStream, std::function<void(DisposeCallbackStream&)> disposeCallback = {});
		~DisposeCallbackStream();

		///	@brief	是否有释放回调
		nBool HasDisposeCallback() const noexcept;

		///	@brief	直接调用回调，之后不会再被调用
		void CallDisposeCallback();

	private:
		std::function<void(DisposeCallbackStream&)> m_DisposeCallback;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	内存流
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

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	外部内存流
	///	@note	用于使用流的方式操作外部的内存
	////////////////////////////////////////////////////////////////////////////////
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
		natRefPointer<natExternMemoryStream> MapToMemoryStream();
#endif

	private:
		UnsafeHandle m_hFile;
		const nBool m_ShouldDispose;

#ifdef _WIN32
		UnsafeHandle m_hMappedFile;
		natRefPointer<natExternMemoryStream> m_pMappedFile;
		const nBool m_IsAsync;
#else
		nBool m_IsEndOfFile;
#endif

		nString m_Filename;
		nBool m_bReadable, m_bWritable;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	子流
	///	@remark	用于操作另一个流的一部分
	////////////////////////////////////////////////////////////////////////////////
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
		void SetPositionFromBegin(nLen Offset) override;
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

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	标准流
	///	@remark	用于操作标准输入输出
	////////////////////////////////////////////////////////////////////////////////
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

	namespace detail_
	{
		template <typename T, typename = void>
		struct StlIStreamTraits
		{
			static constexpr nBool IsIStream = false;

			[[noreturn]] static nLen Read(T&, nData, nLen)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This stream is not a basic_istream."_nv);
			}

			[[noreturn]] static void ForceRead(T&, nData, nLen)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This stream is not a basic_istream."_nv);
			}

			[[noreturn]] static void Seek(T&, NatSeek, nLong)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This stream is not a basic_istream."_nv);
			}

			[[noreturn]] static void Seek(T&, nLen)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This stream is not a basic_istream."_nv);
			}

			[[noreturn]] static nLen Tell(T&)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This stream is not a basic_istream."_nv);
			}
		};

		template <typename T>
		struct StlIStreamTraits<T, std::enable_if_t<std::is_base_of<std::basic_istream<typename T::char_type, typename T::traits_type>, T>::value>>
		{
			static constexpr nBool IsIStream = true;

			static nLen Read(T& stream, nData data, nLen length)
			{
				return static_cast<nLen>(stream.readsome(reinterpret_cast<typename T::char_type*>(data), static_cast<std::streamsize>(length) / sizeof(typename T::char_type)));
			}

			static void ForceRead(T& stream, nData data, nLen length)
			{
				stream.read(reinterpret_cast<typename T::char_type*>(data), static_cast<std::streamsize>(length) / sizeof(typename T::char_type));
			}

			static void Seek(T& stream, NatSeek origin, nLong off)
			{
				std::ios_base::seekdir seek;
				switch (origin)
				{
				case NatSeek::Beg:
				default:
					assert(origin == NatSeek::Beg && "invalid origin.");
					seek = std::ios_base::beg;
					break;
				case NatSeek::Cur:
					seek = std::ios_base::cur;
					break;
				case NatSeek::End:
					seek = std::ios_base::end;
					break;
				}

				stream.seekg(static_cast<typename T::off_type>(off), seek);
			}

			static void Seek(T& stream, nLen pos)
			{
				stream.seekg(static_cast<typename T::pos_type>(pos));
			}

			static nLen Tell(T& stream)
			{
				return static_cast<nLen>(stream.tellg());
			}
		};

		template <typename T, typename = void>
		struct StlOStreamTraits
		{
			static constexpr nBool IsOStream = false;

			[[noreturn]] static nLen Write(T&, ncData, nLen)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This stream is not a basic_ostream."_nv);
			}

			[[noreturn]] static void Seek(T&, NatSeek, nLong)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This stream is not a basic_ostream."_nv);
			}

			[[noreturn]] static void Seek(T&, nLen)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This stream is not a basic_ostream."_nv);
			}

			[[noreturn]] static nLen Tell(T&)
			{
				nat_Throw(natErrException, NatErr_NotSupport, "This stream is not a basic_ostream."_nv);
			}
		};

		template <typename T>
		struct StlOStreamTraits<T, std::enable_if_t<std::is_base_of<std::basic_ostream<typename T::char_type, typename T::traits_type>, T>::value>>
		{
			static constexpr nBool IsOStream = true;

			static void Write(T& stream, ncData data, nLen length)
			{
				stream.write(reinterpret_cast<const typename T::char_type*>(data), static_cast<std::streamsize>(length) / sizeof(typename T::char_type));
			}

			static void Seek(T& stream, NatSeek origin, nLong off)
			{
				std::ios_base::seekdir seek;
				switch (origin)
				{
				case NatSeek::Beg:
				default:
					assert(origin == NatSeek::Beg && "invalid origin.");
					seek = std::ios_base::beg;
					break;
				case NatSeek::Cur:
					seek = std::ios_base::cur;
					break;
				case NatSeek::End:
					seek = std::ios_base::end;
					break;
				}

				stream.seekp(static_cast<typename T::off_type>(off), seek);
			}

			static void Seek(T& stream, nLen pos)
			{
				stream.seekp(static_cast<typename T::pos_type>(pos));
			}

			static nLen Tell(T& stream)
			{
				return static_cast<nLen>(stream.tellp());
			}
		};

		template <typename T>
		struct StlStreamTraits
		{
			static constexpr nBool IsIStream = StlIStreamTraits<T>::IsIStream;
			static constexpr nBool IsOStream = StlOStreamTraits<T>::IsOStream;

			static_assert(IsIStream || IsOStream, "T is not a stl stream.");

			static nLen Read(T& stream, nData data, nLen length)
			{
				return StlIStreamTraits<T>::Read(stream, data, length);
			}

			static void ForceRead(T& stream, nData data, nLen length)
			{
				StlIStreamTraits<T>::ForceRead(stream, data, length);
			}

			static void Write(T& stream, ncData data, nLen length)
			{
				return StlOStreamTraits<T>::Write(stream, data, length);
			}

			static void Seek(T& stream, NatSeek origin, nLong off)
			{
				std::conditional_t<IsIStream, StlIStreamTraits<T>, StlOStreamTraits<T>>::Seek(stream, origin, off);
			}

			static void Seek(T& stream, nLen pos)
			{
				std::conditional_t<IsIStream, StlIStreamTraits<T>, StlOStreamTraits<T>>::Seek(stream, pos);
			}

			static nLen Tell(T& stream)
			{
				return std::conditional_t<IsIStream, StlIStreamTraits<T>, StlOStreamTraits<T>>::Tell(stream);
			}
		};
	}

	template <typename baseStream>
	class natStlStream
		: public natRefObjImpl<natStlStream<baseStream>, natStream>
	{
	public:
		typedef detail_::StlStreamTraits<baseStream> Traits;

		explicit natStlStream(baseStream& stream)
			: m_StlStream{ stream }
		{	
		}

		baseStream& GetUnderlyingStream() const noexcept
		{
			return m_StlStream;
		}

		nBool CanWrite() const override
		{
			return Traits::IsOStream;
		}

		nBool CanRead() const override
		{
			return Traits::IsIStream;
		}

		nBool CanResize() const override
		{
			return false;
		}

		nBool CanSeek() const override
		{
			return true;
		}

		nBool IsEndOfStream() const override
		{
			return m_StlStream.eof();
		}

		nLen GetSize() const override
		{
			const auto current = Traits::Tell(m_StlStream);
			Traits::Seek(m_StlStream, 0);
			const auto begin = Traits::Tell(m_StlStream);
			Traits::Seek(m_StlStream, NatSeek::End, 0);
			const auto size = Traits::Tell(m_StlStream) - begin;
			Traits::Seek(m_StlStream, current);
			return size;
		}

		void SetSize(nLen /*Size*/) override
		{
			nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
		}

		nLen GetPosition() const override
		{
			return Traits::Tell(m_StlStream);
		}

		void SetPosition(NatSeek Origin, nLong Offset) override
		{
			Traits::Seek(m_StlStream, Origin, Offset);
		}

		void SetPositionFromBegin(nLen pos) override
		{
			Traits::Seek(m_StlStream, pos);
		}

		nLen ReadBytes(nData pData, nLen Length) override
		{
			return Traits::Read(m_StlStream, pData, Length);
		}

		void ForceReadBytes(nData pData, nLen Length) override
		{
			Traits::ForceRead(m_StlStream, pData, Length);
		}

		nLen WriteBytes(ncData pData, nLen Length) override
		{
			const auto pos = Traits::Tell(m_StlStream);
			Traits::Write(m_StlStream, pData, Length);
			return Traits::Tell(m_StlStream) - pos;
		}

		void ForceWriteBytes(ncData pData, nLen Length) override
		{
			nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
		}

		void Flush() override
		{
			m_StlStream.flush();
		}

	private:
		baseStream& m_StlStream;
	};
}
