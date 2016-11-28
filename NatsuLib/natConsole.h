#pragma once
#include "natConfig.h"
#include "natStreamHelper.h"

namespace NatsuLib
{
	class natConsole
	{
	public:
		natConsole();

		void Write(nStrView const& str);
		void WriteLine(nStrView const& str);
		void WriteErr(nStrView const& str);
		void WriteLineErr(nStrView const& str);
		nString ReadLine();

	private:
		natStdStream m_StdIn, m_StdOut, m_StdErr;

		natStreamReader<StringType::Utf8> m_StdInReader;
		natStreamWriter<StringType::Utf8> m_StdOutWriter;
		natStreamWriter<StringType::Utf8> m_StdErrWriter;
	};
}
