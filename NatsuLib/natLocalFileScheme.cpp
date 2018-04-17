#include "stdafx.h"
#include "natLocalFileScheme.h"

using namespace NatsuLib;

nStrView LocalFileScheme::GetSchemeName() const noexcept
{
	return "file";
}

natRefPointer<IRequest> LocalFileScheme::CreateRequest(Uri const& uri)
{
	return make_ref<LocalFileRequest>(uri);
}

natRefPointer<IResponse> LocalFileRequest::GetResponse()
{
	const auto host = m_Uri.GetHost();
	const auto realPath = host.empty() ? nString{ m_Uri.GetPath() } : natUtil::FormatString("{0}/{1}", m_Uri.GetHost(), m_Uri.GetPath());

	return make_ref<LocalFileResponse>(make_ref<natFileStream>(realPath, m_Readable, m_Writable
#ifdef _WIN32
		, m_Async
#endif
			));
}

void LocalFileRequest::SetReadable(nBool value) noexcept
{
	m_Readable = value;
}

nBool LocalFileRequest::IsReadable() const noexcept
{
	return m_Readable;
}

void LocalFileRequest::SetWritable(nBool value) noexcept
{
	m_Writable = value;
}

nBool LocalFileRequest::IsWritable() const noexcept
{
	return m_Writable;
}

#ifdef _WIN32
void LocalFileRequest::SetAsync(nBool value) noexcept
{
	m_Async = value;
}

nBool LocalFileRequest::IsAsync() const noexcept
{
	return m_Async;
}
#endif

LocalFileRequest::LocalFileRequest(Uri const& uri)
	: m_Uri{ uri }, m_Readable{ true }, m_Writable{ false }
#ifdef _WIN32
		, m_Async{ false }
#endif
{
}

natRefPointer<natStream> LocalFileResponse::GetResponseStream()
{
	return m_InternalStream;
}

LocalFileResponse::LocalFileResponse(natRefPointer<natFileStream> stream)
	: m_InternalStream{ std::move(stream) }
{
}
