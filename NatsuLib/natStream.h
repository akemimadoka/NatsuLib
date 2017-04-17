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
		virtual nLen WriteBytes(ncData pData, nLen Length) = 0;

		///	@brief		ǿ��д���ֽ�����
		///	@param[in]	pData	���ݻ�����
		///	@param[in]	Length	д��ĳ���
		///	@note		�ظ�д�����ֱ���ɹ�д����ֽ�����С��LengthΪֹ��ע�Ȿ�������������ѭ��
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
	///	@note	��װ������һ���ڲ�����Ĭ�����в���ֱ��ת�����ڲ���\n
	///			��ͨ���̳д�����ɶ����ݵ����⴦��\n
	///			ע������ʱ�����������������ִ�У�֮��Ż������ڲ���
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
		nBool EnumUnderlyingStream(std::function<bool(natWrappedStream&)> const& enumerator) const;

		///	@brief	��������ڲ���������ڲ�����������T�򷵻���������޷����������͵��ڲ����򷵻�nullptr
		///	@tparam	T	Ҫ��õ��ڲ���������
		template <typename T>
		std::enable_if_t<std::is_base_of<natStream, T>::value, natRefPointer<T>> GetUnderlyingStreamAs() const noexcept
		{
			return GetUnderlyingStreamAs(typeid(T));
		}

		///	@see GetUnderlyingStreamAs{T}
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
	///	@brief	NatsuLib�ڴ���ʵ��
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
