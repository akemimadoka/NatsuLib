#pragma once
#include "natConfig.h"
#include "natStream.h"

namespace NatsuLib
{
	namespace detail_
	{
		struct DeflateStreamImpl;
	}

	class natDeflateStream
		: public natRefObjImpl<natStream>
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

		explicit natDeflateStream(natRefPointer<natStream> stream);
		natDeflateStream(natRefPointer<natStream> stream, CompressionLevel compressionLevel);
		~natDeflateStream();

		natRefPointer<natStream> GetUnderlyingStream() const noexcept;

		nBool CanWrite() const override;
		nBool CanRead() const override;
		nBool CanResize() const override;
		nBool CanSeek() const override;
		nBool IsEndOfStream() const override;
		nLen GetSize() const override;
		void SetSize(nLen /*Size*/) override;
		nLen GetPosition() const override;
		void SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override;
		nLen ReadBytes(nData pData, nLen Length) override;
		nLen WriteBytes(ncData pData, nLen Length) override;

		void Flush() override;

	private:
		natRefPointer<natStream> m_InternalStream;
		nByte m_Buffer[DefaultBufferSize];
		std::unique_ptr<detail_::DeflateStreamImpl> m_Impl;
		nBool m_WroteData;

		nLen writeAll();
	};
}
