////////////////////////////////////////////////////////////////////////////////
///	@file	natStream.h
///	@brief	NatsuLib��
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
	///	@brief	Ѱַ�ο�λ��
	////////////////////////////////////////////////////////////////////////////////
	enum class NatSeek
	{
		Beg,	///< @brief	����ͷ
		Cur,	///< @brief	��ǰ��λ��
		End		///< @brief	����β
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	��
	////////////////////////////////////////////////////////////////////////////////
	struct natStream
		: natRefObj
	{
		enum
		{
			DefaultCopyToBufferSize = 1024,
		};

		virtual ~natStream();

		///	@brief		���Ƿ��д
		virtual nBool CanWrite() const = 0;

		///	@brief		���Ƿ�ɶ�
		virtual nBool CanRead() const = 0;

		///	@brief		���Ƿ�����·����С
		virtual nBool CanResize() const = 0;

		///	@brief		���Ƿ��Ѱַ
		virtual nBool CanSeek() const = 0;

		///	@brief		���Ƿ��ѵ����β
		virtual nBool IsEndOfStream() const = 0;

		///	@brief		������Ĵ�С
		virtual nLen GetSize() const = 0;

		///	@brief		���·������Ĵ�С
		///	@note		���·���ɹ��Ժ�ָ��λ�����Ŀ�ͷ
		virtual void SetSize(nLen Size) = 0;

		///	@brief		��ö�дָ���λ��
		virtual nLen GetPosition() const = 0;

		///	@brief		���ö�дָ���λ��
		///	@param[in]	Origin	Ѱַ�ο�λ��
		///	@param[in]	Offset	ƫ��
		virtual void SetPosition(NatSeek Origin, nLong Offset) = 0;

		///	@brief		�ӿ�ͷ���ö�дָ���λ��
		///	@param[in]	Offset	ƫ��
		virtual void SetPositionFromBegin(nLen Offset);

		/// @brief		�����ж�ȡһ���ֽ�
		virtual nByte ReadByte();

		///	@brief		��ȡ�ֽ�����
		///	@param[out]	pData	���ݻ�����
		///	@param[in]	Length	��ȡ�ĳ���
		///	@return		ʵ�ʶ�ȡ����
		virtual nLen ReadBytes(nData pData, nLen Length) = 0;

		///	@brief		ǿ�ƶ�ȡ�ֽ�����
		///	@param[out]	pData	���ݻ�����
		///	@param[in]	Length	��ȡ�ĳ���
		///	@note		�ظ���ȡ����ֱ���ɹ���ȡ���ֽ�����С��LengthΪֹ��ע�Ȿ�������������ѭ��
		virtual void ForceReadBytes(nData pData, nLen Length);

		/// @brief		�첽��ȡ�ֽ�����
		/// @param[out]	pData	���ݻ�����
		/// @param[in]	Length	��ȡ�ĳ���
		/// @return		ʵ�ʶ�ȡ����
		virtual std::future<nLen> ReadBytesAsync(nData pData, nLen Length);

		/// @brief		������д��һ���ֽ�
		virtual void WriteByte(nByte byte);

		///	@brief		д���ֽ�����
		///	@param[in]	pData	���ݻ�����
		///	@param[in]	Length	д��ĳ���
		///	@return		ʵ��д�볤��
		///	@note		�����޷�������д���ֽ����������������ص�ֵ������
		virtual nLen WriteBytes(ncData pData, nLen Length) = 0;

		///	@brief		ǿ��д���ֽ�����
		///	@param[in]	pData	���ݻ�����
		///	@param[in]	Length	д��ĳ���
		///	@note		�ظ�д�����ֱ���ɹ�д����ֽ�����С��LengthΪֹ��ע�Ȿ�������������ѭ��
		///	@warning	���Ƕ��������������ã������޷�������д���ֽ���������ֹʹ�ñ�����
		virtual void ForceWriteBytes(ncData pData, nLen Length);

		///	@brief		�첽д���ֽ�����
		///	@param[in]	pData	���ݻ�����
		///	@param[in]	Length	д��ĳ���
		///	@return		ʵ��д�볤��
		virtual std::future<nLen> WriteBytesAsync(ncData pData, nLen Length);

		///	@brief		�����е����ݸ��Ƶ���һ��
		///	@param[in]	other	Ҫ���Ƶ�����
		///	@return		��ʵ�ʶ�ȡ����
		///	@note		��ȡ���Ȳ���ζ�ųɹ�д�뵽��һ���ĳ���
		virtual nLen CopyTo(natRefPointer<natStream> const& other);

		///	@brief		ˢ����
		///	@note		�����л�����Ƶ�����Ч��������
		virtual void Flush() = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	��װ��
	///	@note	��װ������һ���ڲ�����Ĭ�ϳ���CopyTo��������в���ֱ��ת�����ڲ���\n
	///			��ͨ���̳д�����ɶ����ݵ����⴦��\n
	///			ע������ʱ�����������������ִ�У�֮��Ż��ͷ��ڲ���
	////////////////////////////////////////////////////////////////////////////////
	class natWrappedStream
		: public natRefObjImpl<natWrappedStream, natStream>
	{
	public:
		explicit natWrappedStream(natRefPointer<natStream> stream);
		~natWrappedStream();

		///	@brief	����ڲ���
		virtual natRefPointer<natStream> GetUnderlyingStream() const noexcept;

		///	@brief	����ö���ڲ�����ֱ���ڲ�������natWrappedStream����enumerator����trueΪֹ
		///	@param	enumerator	ö�ٺ���������true������ֹͣö��
		///	@note	�����γɻ��Ŀ��ܣ���ʱ���������ܻ������ѭ��
		///	@return	ö���Ƿ�����enumerator����true����ֹ
		nBool EnumUnderlyingStream(std::function<nBool(natWrappedStream&)> const& enumerator) const;

		///	@brief	��������ڲ���������ڲ�����������T�򷵻���������޷����������͵��ڲ����򷵻�nullptr
		///	@tparam	T	Ҫ��õ��ڲ���������
		template <typename T>
		std::enable_if_t<std::is_base_of<natStream, T>::value, natRefPointer<T>> GetUnderlyingStreamAs() const noexcept
		{
			return GetUnderlyingStreamAs(typeid(T));
		}

		natRefPointer<natStream> GetUnderlyingStreamAs(std::type_info const& typeinfo) const noexcept;

		///	@brief	��������ڲ����������������natWrappedStream�򷵻���
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
	///	@brief	�ͷŻص���
	///	@note	�������ͷ�ʱ�������⴦���������������ֱ��ת�����ڲ���
	////////////////////////////////////////////////////////////////////////////////
	class DisposeCallbackStream final
		: public natRefObjImpl<DisposeCallbackStream, natWrappedStream>
	{
	public:
		explicit DisposeCallbackStream(natRefPointer<natStream> internalStream, std::function<void(DisposeCallbackStream&)> disposeCallback = {});
		~DisposeCallbackStream();

		///	@brief	�Ƿ����ͷŻص�
		nBool HasDisposeCallback() const noexcept;

		///	@brief	ֱ�ӵ��ûص���֮�󲻻��ٱ�����
		void CallDisposeCallback();

	private:
		std::function<void(DisposeCallbackStream&)> m_DisposeCallback;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	�ڴ���
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

		///	@brief	���ʵ���������ݴ�С������������
		nLen GetSize() const override;

		///	@brief	�������Ĵ�С
		///	@note	�޷�ͨ�����ý�С�Ĵ�С���ض����ݣ�ͨ��SetSizeֻ�������������޷���С
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
	///	@brief	�ⲿ�ڴ���
	///	@note	����ʹ�����ķ�ʽ�����ⲿ���ڴ�
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
	///	@brief	NatsuLib�ļ���ʵ��
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

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	����
	///	@remark	���ڲ�����һ������һ����
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
	///	@brief	��׼��
	///	@remark	���ڲ�����׼�������
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
