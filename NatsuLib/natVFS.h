#pragma once

#include "natString.h"
#include "natMisc.h"
#include "natRefObj.h"
#include "natStream.h"
#include <unordered_map>

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	ͨ����Դ��ʶ��
	////////////////////////////////////////////////////////////////////////////////
	class Uri final
	{
	public:
		// �ƺ�û�ã�
		static const nStrView SchemeDelimiter;

		explicit Uri(nString uri);

		Uri(Uri const& other);
		Uri(Uri&& other) noexcept;

		Uri& operator=(Uri const& other);
		Uri& operator=(Uri&& other) noexcept;

		nStrView GetScheme() const noexcept;
		nStrView GetUser() const noexcept;
		nStrView GetPassword() const noexcept;
		nStrView GetHost() const noexcept;
		Optional<nuShort> GetPort() const noexcept;
		nStrView GetPath() const noexcept;
		nStrView GetQuery() const noexcept;
		nStrView GetFragment() const noexcept;

		nStrView GetUnderlyingString() const noexcept;

	private:
		// Ϊʲô��Ҫд���������
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

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	����ӿ�
	////////////////////////////////////////////////////////////////////////////////
	struct IRequest
		: natRefObj
	{
		~IRequest();

		virtual natRefPointer<IResponse> GetResponse() = 0;
		virtual std::future<natRefPointer<IResponse>> GetResponseAsync();
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	�ظ��ӿ�
	////////////////////////////////////////////////////////////////////////////////
	struct IResponse
		: natRefObj
	{
		~IResponse();

		virtual natRefPointer<natStream> GetResponseStream() = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	�����ӿ�
	////////////////////////////////////////////////////////////////////////////////
	struct IScheme
		: natRefObj
	{
		~IScheme();

		///	@brief	��÷�������
		///	@note	��õ�nStrView�������������Scheme��������������������Ч���ַ���
		virtual nStrView GetSchemeName() const noexcept = 0;
		virtual natRefPointer<IRequest> CreateRequest(Uri const& uri) = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	�����ļ�ϵͳ
	////////////////////////////////////////////////////////////////////////////////
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

namespace std
{
	template <>
	struct hash<NatsuLib::Uri>
	{
		std::size_t operator()(NatsuLib::Uri const& uri) const noexcept
		{
			return hash<nStrView>{}(uri.GetUnderlyingString());
		}
	};
}
