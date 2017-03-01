#include "stdafx.h"
#include "natVFS.h"
#include "natException.h"
#include "natLocalFileScheme.h"

using namespace NatsuLib;

namespace
{
	// 无视溢出，谁溢出谁治理
	// 假设begin和end总是指向相同的内存块或其后一字节
	template <typename CharType, typename T>
	std::enable_if_t<std::is_arithmetic<T>::value, bool> TryParseNumber(const CharType* begin, const CharType* end, T& num) noexcept
	{
		auto pRead = begin;
		const auto pEnd = end;
		assert(pRead <= pEnd);

		T tmpNum{};
		for (; pRead < pEnd; ++pRead)
		{
			const auto digit = static_cast<std::make_unsigned_t<CharType>>(*pRead - CharType{ '0' });
			if (digit > 9)
			{
				return false;
			}

			tmpNum *= 10;
			tmpNum += digit;
		}

		num = tmpNum;
		return true;
	}
}

const nStrView Uri::SchemeDelimiter{ "://" };

Uri::Uri(nString uri)
	: m_UriInfo{ std::move(uri) }
{
	ParseUri();
}

Uri::Uri(Uri const& other)
	: Uri(other.m_UriInfo.UriString)
{
}

Uri::Uri(Uri&& other) noexcept
	: Uri(std::move(other.m_UriInfo.UriString))
{
}

Uri& Uri::operator=(Uri const& other)
{
	m_UriInfo = { other.m_UriInfo.UriString };
	ParseUri();

	return *this;
}

Uri& Uri::operator=(Uri&& other) noexcept
{
	m_UriInfo = { std::move(other.m_UriInfo.UriString) };
	ParseUri();

	return *this;
}

nStrView Uri::GetScheme() const noexcept
{
	return m_UriInfo.Scheme;
}

nStrView Uri::GetUser() const noexcept
{
	return m_UriInfo.User;
}

nStrView Uri::GetPassword() const noexcept
{
	return m_UriInfo.Password;
}

nStrView Uri::GetHost() const noexcept
{
	return m_UriInfo.Host;
}

Optional<nuShort> Uri::GetPort() const noexcept
{
	return m_UriInfo.Port;
}

nStrView Uri::GetPath() const noexcept
{
	return m_UriInfo.Path;
}

nStrView Uri::GetQuery() const noexcept
{
	return m_UriInfo.Query;
}

nStrView Uri::GetFragment() const noexcept
{
	return m_UriInfo.Fragment;
}

nStrView Uri::GetUnderlyingString() const noexcept
{
	return m_UriInfo.UriString;
}

void Uri::ParseUri()
{
	enum class State
	{
		Scheme,
		User,
		Password,
		Host,
		Port,
		Path,
		Query,
		Fragment,

		Accept,
		Reject
	} state { State::Scheme };

	// 假设pRead, pLastBegin和pEnd都是RandomAccessIterator，且都指向同一内存块或其后恰好一字节处

	auto pRead = m_UriInfo.UriString.cbegin();
	auto pLastBegin = pRead;
	const auto pEnd = m_UriInfo.UriString.cend();

	nStrView user; // 临时存储用户名，因为可能并非真正的用户名
	nuShort port;

	for (; pRead < pEnd; ++pRead)
	{
		switch (state)
		{
		case State::Scheme:
			if (*pRead == ':')
			{
				if (pEnd - pRead >= 3 && *(pRead + 1) == '/' && *(pRead + 2) == '/')
				{
					m_UriInfo.Scheme = nStrView{ pLastBegin, pRead };
					pLastBegin = pRead + 3;
					pRead += 2; // 循环会再后移一位
					state = State::User;
				}
				else
				{
					pRead = pEnd - 1; // 立即结束循环
					state = State::Reject;
				}
			}
			break;
		case State::User:
			switch (*pRead)
			{
			case ':':
				user = nStrView{ pLastBegin, pRead };
				pLastBegin = pRead + 1;
				state = State::Password;
				break;
			case '@':
				user = nStrView{ pLastBegin, pRead };
				m_UriInfo.User = user;
				pLastBegin = pRead + 1;
				state = State::Host;
				break;
			case '/':
				// 读取的是Host而不是User
				m_UriInfo.Host = nStrView{ pLastBegin, pRead };
				pLastBegin = pRead + 1;
				state = State::Path;
				break;
			default:
				break;
			}
			break;
		case State::Password:
			switch (*pRead)
			{
			case '@':
				m_UriInfo.User = user;
				m_UriInfo.Password = nStrView{ pLastBegin, pRead };
				pLastBegin = pRead + 1;
				state = State::Host;
				break;
			case '/':
				// 读取的是Port而不是Password，之前读取的是Host而不是User
				// **你被骗了！**
				m_UriInfo.Host = user;
				if (!TryParseNumber(pLastBegin, pRead, port))
				{
					pRead = pEnd - 1;
					state = State::Reject;
					break;
				}
				m_UriInfo.Port = port;
				pLastBegin = pRead + 1;
				state = State::Path;
				break;
			default:
				break;
			}
			break;
		case State::Host:
			switch (*pRead)
			{
			case ':':
				m_UriInfo.Host = nStrView{ pLastBegin, pRead };
				pLastBegin = pRead + 1;
				state = State::Port;
				break;
			case '/':
				m_UriInfo.Host = nStrView{ pLastBegin, pRead };
				pLastBegin = pRead + 1;
				state = State::Path;
				break;
			default:
				break;
			}
			break;
		case State::Port:
			switch (*pRead)
			{
			case '/':
				if (!TryParseNumber(pLastBegin, pRead, port))
				{
					pRead = pEnd - 1;
					state = State::Reject;
					break;
				}
				m_UriInfo.Port = port;
				pLastBegin = pRead + 1;
				state = State::Path;
				break;
			default:
				break;
			}
			break;
		case State::Path:
			switch (*pRead)
			{
			case '?':
				m_UriInfo.Path = nStrView{ pLastBegin, pRead };
				pLastBegin = pRead + 1;
				state = State::Query;
				break;
			case '#':
				m_UriInfo.Path = nStrView{ pLastBegin, pRead };
				pLastBegin = pRead + 1;
				state = State::Fragment;
				break;
			default:
				break;
			}
			break;
		case State::Query:
			switch (*pRead)
			{
			case '#':
				m_UriInfo.Query = nStrView{ pLastBegin, pRead };
				pLastBegin = pRead + 1;
				state = State::Fragment;
				break;
			default:
				break;
			}
			break;
		case State::Fragment:
			break;
		case State::Accept:
			break;
		case State::Reject:
			pRead = pEnd - 1;
			break;
		default:
			assert(!"This should never happen.");
			break;
		}
	}

	switch (state)
	{
	case State::Scheme:
		m_UriInfo.Scheme = nStrView{ pLastBegin, pRead };
		break;
	case State::User:
		m_UriInfo.User = nStrView{ pLastBegin, pRead };
		break;
	case State::Password:
		m_UriInfo.Password = nStrView{ pLastBegin, pRead };
		break;
	case State::Host:
		m_UriInfo.Host = nStrView{ pLastBegin, pRead };
		break;
	case State::Port:
		if (!TryParseNumber(pLastBegin, pRead, port))
		{
			nat_Throw(natException, "{0} is not a valid uri."_nv, m_UriInfo.UriString);
		}
		m_UriInfo.Port = port;
		break;
	case State::Path:
		m_UriInfo.Path = nStrView{ pLastBegin, pRead };
		break;
	case State::Query:
		m_UriInfo.Query = nStrView{ pLastBegin, pRead };
		break;
	case State::Fragment:
		m_UriInfo.Fragment = nStrView{ pLastBegin, pRead };
		break;
	case State::Accept:
		return;
	case State::Reject:
		nat_Throw(natException, "{0} is not a valid uri."_nv, m_UriInfo.UriString);
	default:
		assert(!"This should never happen.");
		break;
	}
}

IRequest::~IRequest()
{
}

std::future<natRefPointer<IResponse>> IRequest::GetResponseAsync()
{
	return std::async([this]
	{
		return GetResponse();
	});
}

IResponse::~IResponse()
{
}

IScheme::~IScheme()
{
}

natVFS::natVFS()
{
	RegisterScheme(make_ref<LocalFileScheme>());
}

natVFS::~natVFS()
{
}

void natVFS::RegisterScheme(natRefPointer<IScheme> scheme)
{
	nBool succeed;
	tie(std::ignore, succeed) = m_SchemeMap.emplace(scheme->GetSchemeName(), std::move(scheme));
	if (!succeed)
	{
		nat_Throw(natErrException, NatErr_Duplicated, "Register scheme failed: duplicated scheme name."_nv);
	}
}

void natVFS::UnregisterScheme(nStrView name)
{
	m_SchemeMap.erase(name);
}

natRefPointer<IScheme> natVFS::GetScheme(nStrView name)
{
	const auto iter = m_SchemeMap.find(name);
	return iter != m_SchemeMap.end() ? iter->second : nullptr;
}

natRefPointer<IRequest> natVFS::CreateRequest(Uri const& uri)
{
	const auto scheme = GetScheme(uri.GetScheme());
	return scheme ? scheme->CreateRequest(uri) : nullptr;
}

natRefPointer<IRequest> natVFS::CreateRequest(nStrView const& uriString)
{
	return CreateRequest(Uri{ uriString });
}
