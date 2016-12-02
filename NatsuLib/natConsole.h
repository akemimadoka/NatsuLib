#pragma once
#include "natConfig.h"
#include "natStreamHelper.h"

namespace NatsuLib
{
	class natConsole
	{
	public:
		static constexpr StringType Encoding =
#ifdef _WIN32
			StringType::Wide;
#else
			StringType::Utf8;
#endif

		natConsole();

		template <StringType stringType>
		void Write(StringView<stringType> const& str)
		{
			Write(String<Encoding>{str}.GetView());
		}

		void Write(StringView<Encoding> const& str);

		template <StringType stringType>
		void Write(String<stringType> const& str)
		{
			Write(str.GetView());
		}

		template <StringType stringType>
		void WriteLine(StringView<stringType> const& str)
		{
			WriteLine(String<Encoding>{str}.GetView());
		}

		void WriteLine(StringView<Encoding> const& str);

		template <StringType stringType>
		void WriteLine(String<stringType> const& str)
		{
			WriteLine(str.GetView());
		}

		template <StringType stringType>
		void WriteErr(StringView<stringType> const& str)
		{
			WriteErr(String<Encoding>{str}.GetView());
		}

		void WriteErr(StringView<Encoding> const& str);

		template <StringType stringType>
		void WriteErr(String<stringType> const& str)
		{
			WriteErr(str.GetView());
		}

		template <StringType stringType>
		void WriteLineErr(StringView<stringType> const& str)
		{
			WriteLineErr(String<Encoding>{str}.GetView());
		}

		void WriteLineErr(StringView<Encoding> const& str);

		template <StringType stringType>
		void WriteLineErr(String<stringType> const& str)
		{
			WriteLineErr(str.GetView());
		}

		template <StringType stringType = Encoding>
		String<stringType> ReadLine()
		{
			return m_StdInReader.ReadLine();
		}

	private:
		natStdStream m_StdIn, m_StdOut, m_StdErr;

		natStreamReader<Encoding> m_StdInReader;
		natStreamWriter<Encoding> m_StdOutWriter;
		natStreamWriter<Encoding> m_StdErrWriter;
	};
}
