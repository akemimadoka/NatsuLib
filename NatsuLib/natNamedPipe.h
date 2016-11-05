#pragma once
#include "natStream.h"
#include <future>

#pragma push_macro("max")
#undef max

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
		: public natRefObjImpl<natStream>
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

		natNamedPipeServerStream(ncTStr Pipename, PipeDirection Direction, nuInt MaxInstances, nuInt OutBuffer = 1024, nuInt InBuffer = 1024, nuInt TimeOut = 0, PipeMode TransmissionMode = PipeMode::Message, PipeMode ReadMode = PipeMode::Message, PipeOptions Options = PipeOptions::WriteThrough);
		~natNamedPipeServerStream();

		nBool CanWrite() const override
		{
			return m_bWritable;
		}

		nBool CanRead() const override
		{
			return m_bReadable;
		}

		nBool CanResize() const override
		{
			return false;
		}

		nBool CanSeek() const override
		{
			return false;
		}

		nLen GetSize() const override
		{
			return 0ul;
		}

		void SetSize(nLen /*Size*/) override
		{
			nat_Throw(natErrException, NatErr_NotSupport, _T("This type of stream does not support SetSize."));
		}

		nLen GetPosition() const override
		{
			return 0ul;
		}

		void SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override
		{
			nat_Throw(natErrException, NatErr_NotSupport, _T("This type of stream does not support SetPosition."));
		}

		nLen ReadBytes(nData pData, nLen Length) override;
		std::future<nLen> ReadBytesAsync(nData pData, nLen Length) override;
		nLen WriteBytes(ncData pData, nLen Length) override;
		std::future<nLen> WriteBytesAsync(ncData pData, nLen Length) override;
		void Flush() override;

		void WaitForConnection();
		std::future<void> WaitForConnectionAsync();

		nBool IsAsync() const
		{
			return m_bAsync;
		}

		nBool IsConnected() const
		{
			return m_bConnected;
		}

		nBool IsMessageComplete() const
		{
			return m_bMessageComplete;
		}

	private:
		UnsafeHandle m_hPipe;
		nBool m_bAsync, m_bConnected, m_bMessageComplete, m_bReadable, m_bWritable;
	};

	class natNamedPipeClientStream
		: public natRefObjImpl<natStream>
	{
	public:
		enum TimeOut : nuInt
		{
			Infinity = std::numeric_limits<nuInt>::max(),
		};

		natNamedPipeClientStream(ncTStr Pipename, nBool bReadable, nBool bWritable);
		~natNamedPipeClientStream() = default;

		nBool CanResize() const override
		{
			return false;
		}

		nBool CanSeek() const override
		{
			return false;
		}

		nLen GetSize() const override
		{
			return 0ul;
		}

		void SetSize(nLen /*Size*/) override
		{
			nat_Throw(natErrException, NatErr_NotSupport, _T("This type of stream does not support SetSize."));
		}

		nLen GetPosition() const override
		{
			return 0ul;
		}

		void SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override
		{
			nat_Throw(natErrException, NatErr_NotSupport, _T("This type of stream does not support SetPosition."));
		}

		nBool CanWrite() const override;
		nBool CanRead() const override;
		nLen ReadBytes(nData pData, nLen Length) override;
		std::future<nLen> ReadBytesAsync(nData pData, nLen Length) override;
		nLen WriteBytes(ncData pData, nLen Length) override;
		std::future<nLen> WriteBytesAsync(ncData pData, nLen Length) override;
		void Flush() override;

		void Wait(nuInt timeOut = Infinity);

	private:
		std::unique_ptr<natFileStream> m_InternalStream;
		nTString m_PipeName;
		nBool m_bReadable, m_bWritable;
	};
}

#pragma pop_macro("max")
