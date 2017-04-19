#pragma once
#include "natConfig.h"
#include "natStream.h"

namespace NatsuLib
{
	namespace detail_
	{
		struct DeflateStreamImpl;
	}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	压缩流
	///	@remark	使用deflate算法对数据进行压缩
	////////////////////////////////////////////////////////////////////////////////
	class natDeflateStream
		: public natRefObjImpl<natDeflateStream, natWrappedStream>, public nonmovable
	{
		enum : size_t
		{
			DefaultBufferSize = 8192,
		};

	public:
		enum class CompressionLevel
		{
			Optimal = 0,
			Fastest = 1,
			NoCompression = 2
		};

		explicit natDeflateStream(natRefPointer<natStream> stream, nBool useHeader = false);
		natDeflateStream(natRefPointer<natStream> stream, CompressionLevel compressionLevel, nBool useHeader = false);
		~natDeflateStream();

		nBool CanWrite() const override;
		nBool CanRead() const override;
		nBool CanResize() const override;
		nBool CanSeek() const override;
		nLen GetSize() const override;
		void SetSize(nLen /*Size*/) override;
		nLen GetPosition() const override;
		void SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override;
		nLen ReadBytes(nData pData, nLen Length) override;
		nLen WriteBytes(ncData pData, nLen Length) override;
		void Flush() override;

	private:
		nByte m_Buffer[DefaultBufferSize];
		std::unique_ptr<detail_::DeflateStreamImpl> m_Impl;
		nBool m_WroteData;

		nLen writeAll();
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	Crc32流
	////////////////////////////////////////////////////////////////////////////////
	class natCrc32Stream
		: public natRefObjImpl<natCrc32Stream, natWrappedStream>
	{
	public:
		explicit natCrc32Stream(natRefPointer<natStream> stream);
		~natCrc32Stream();

		///	@brief	获得已输入数据的Crc32
		nuInt GetCrc32() const noexcept;

		nBool CanWrite() const override;
		nBool CanRead() const override;
		nBool CanResize() const override;
		nBool CanSeek() const override;
		nLen GetSize() const override;
		void SetSize(nLen /*Size*/) override;
		nLen GetPosition() const override;
		void SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override;
		nLen ReadBytes(nData pData, nLen Length) override;
		nLen WriteBytes(ncData pData, nLen Length) override;

	private:
		nuInt m_Crc32;
		nLen m_CurrentPosition;
	};
}
