#pragma once

#include "natString.h"
#include "natMisc.h"
#include "natRefObj.h"
#include "natStream.h"
#include <unordered_map>

namespace NatsuLib
{
	class Uri final
	{
	public:
		// 似乎没用？
		static const nStrView SchemeDelimiter;

		explicit Uri(nString uri);

		Uri(Uri const& other);
		Uri(Uri&& other) noexcept;

		Uri& operator=(Uri const& other);
		Uri& operator=(Uri&& other) noexcept;

		nStrView GetScheme() const;
		nStrView GetUser() const;
		nStrView GetPassword() const;
		nStrView GetHost() const;
		Optional<nuShort> GetPort() const;
		nStrView GetPath() const;
		nStrView GetQuery() const;
		nStrView GetFragment() const;

		nStrView GetUnderlyingString() const noexcept;

	private:
		// 为什么我要写这个东西？
		struct UriInfo
		{
			nString UriString;

			nStrView Scheme;
			nStrView User;
			nStrView Password;
			nStrView Host;
			Optional<nuShort> Port;
			nStrView Path;
			nStrView Query;
			nStrView Fragment;
		};

		UriInfo m_UriInfo;

		void ParseUri();
	};

	struct IResponse;

	struct IRequest
		: natRefObj
	{
		virtual natRefPointer<IResponse> GetResponse() = 0;
		virtual std::future<natRefPointer<IResponse>> GetResponseAsync();
	};

	struct IResponse
		: natRefObj
	{
		virtual natRefPointer<natStream> GetResponseStream() = 0;
	};

	struct IScheme
		: natRefObj
	{
		///	@brief	获得方案名称
		///	@note	获得的nStrView必须至少在这个Scheme的整个生存期内引用有效的字符串
		virtual nStrView GetSchemeName() const noexcept = 0;
		virtual natRefPointer<IRequest> CreateRequest(Uri const& uri) = 0;
	};

	class natVFS final
	{
	public:
		natVFS();
		~natVFS();

		void RegisterScheme(natRefPointer<IScheme> scheme);
		void UnregisterScheme(nStrView name);

		natRefPointer<IScheme> GetScheme(nStrView name);
		natRefPointer<IRequest> CreateRequest(Uri const& uri);
		natRefPointer<IRequest> CreateRequest(nStrView const& uriString);

	private:
		std::unordered_map<nStrView, natRefPointer<IScheme>> m_SchemeMap;
	};
}
