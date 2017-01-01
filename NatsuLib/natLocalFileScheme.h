#pragma once
#include "natVFS.h"

namespace NatsuLib
{
	class LocalFileScheme final
		: public natRefObjImpl<IScheme>
	{
	public:
		nStrView GetSchemeName() const noexcept override;
		natRefPointer<IRequest> CreateRequest(Uri const& uri) override;
	};

	class LocalFileRequest final
		: public natRefObjImpl<IRequest>
	{
	public:
		explicit LocalFileRequest(Uri const& uri);

		natRefPointer<IResponse> GetResponse() override;

		void SetReadable(nBool value) noexcept;
		nBool IsReadable() const noexcept;

		void SetWritable(nBool value) noexcept;
		nBool IsWritable() const noexcept;

#ifdef _WIN32
		void SetAsync(nBool value) noexcept;
		nBool IsAsync() const noexcept;
#endif

	private:
		Uri m_Uri;
		nBool m_Readable;
		nBool m_Writable;
#ifdef _WIN32
		nBool m_Async;
#endif
	};

	class LocalFileResponse final
		: public natRefObjImpl<IResponse>
	{
	public:
		explicit LocalFileResponse(natRefPointer<natFileStream> stream);

		natRefPointer<natStream> GetResponseStream() override;

	private:
		natRefPointer<natFileStream> m_InternalStream;
	};
}
