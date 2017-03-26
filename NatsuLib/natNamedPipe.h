#pragma once
#include "natStream.h"
#include <future>

#ifdef _MSC_VER
#	pragma push_macro("max")
#	undef max
#endif

namespace NatsuLib
{
	enum class PipeDirection
	{
		In = 1,
		Out = 2,
		Inout = In | Out,
	};

	enum class PipeMode
	{
		Byte,
		Message,
	};

	enum class PipeOptions
	{
		None = 0,

		Asynchronous = 1,
		WriteThrough = 2,
	};

	class natNamedPipeServerStream
		: public natRefObjImpl<natNamedPipeServerStream, natStream>
	{
	public:
#ifdef _WIN32
		typedef HANDLE UnsafeHandle;
#else
		typedef nUnsafePtr<void> UnsafeHandle;
#endif

		enum : nuInt
		{
			MaxAllowedServerInstances = std::numeric_limits<nuInt>::max(),
		};

		natNamedPipeServerStream(nStrView Pipename, PipeDirection Direction, nuInt MaxInstances, nuInt OutBuffer = 1024, nuInt InBuffer = 1024, nuInt TimeOut = 0, PipeMode TransmissionMode = PipeMode::Message, PipeMode ReadMode = PipeMode::Message, PipeOptions Options = PipeOptions::WriteThrough);
		~natNamedPipeServerStream();

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

		void WaitForConnection();
		std::future<void> WaitForConnectionAsync();

		nBool IsAsync() const noexcept
		{
			return m_bAsync;
		}

		nBool IsConnected() const noexcept
		{
			return m_bConnected;
		}

		nBool IsMessageComplete() const noexcept
		{
			return m_bMessageComplete;
		}

	private:
		UnsafeHandle m_hPipe;
		nBool m_bAsync, m_bConnected, m_bMessageComplete, m_bReadable, m_bWritable;
	};

	class natNamedPipeClientStream
		: public natRefObjImpl<natNamedPipeClientStream, natStream>
	{
	public:
		enum TimeOut : nuInt
		{
			Infinity = std::numeric_limits<nuInt>::max(),
		};

		natNamedPipeClientStream(nStrView Pipename, nBool bReadable, nBool bWritable);
		~natNamedPipeClientStream();

		nBool CanResize() const override;
		nBool CanSeek() const override;
		nLen GetSize() const override;
		nBool IsEndOfStream() const override;
		void SetSize(nLen /*Size*/) override;
		nLen GetPosition() const override;
		void SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override;
		nBool CanWrite() const override;
		nBool CanRead() const override;
		nByte ReadByte() override;
		nLen ReadBytes(nData pData, nLen Length) override;
		std::future<nLen> ReadBytesAsync(nData pData, nLen Length) override;
		void WriteByte(nByte byte) override;
		nLen WriteBytes(ncData pData, nLen Length) override;
		std::future<nLen> WriteBytesAsync(ncData pData, nLen Length) override;
		void Flush() override;

		void Wait(nuInt timeOut = Infinity);

	private:
		natRefPointer<natFileStream> m_InternalStream;
		nString m_PipeName;
		nBool m_bReadable, m_bWritable;
	};
}

#ifdef _MSC_VER
#	pragma pop_macro("max")
#endif
