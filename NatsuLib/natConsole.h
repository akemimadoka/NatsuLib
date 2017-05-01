#pragma once
#include "natConfig.h"
#include "natStreamHelper.h"
#include "natMisc.h"

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

		enum class ConsoleColor
		{
			Black = 0,
			DarkBlue = 1,
			DarkGreen = 2,
			DarkCyan = 3,
			DarkRed = 4,
			DarkMagenta = 5,
			DarkYellow = 6,
			Gray = 7,
			DarkGray = 8,
			Blue = 9,
			Green = 10,
			Cyan = 11,
			Red = 12,
			Magenta = 13,
			Yellow = 14,
			White = 15
		};

		enum class ConsoleColorTarget
		{
			Background,
			Foreground
		};

		natConsole();

		nString GetTitle() const;
		void SetTitle(nStrView title);

		ConsoleColor GetColor(ConsoleColorTarget target) const;
		void SetColor(ConsoleColorTarget target, ConsoleColor color);
		void ResetColor();

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

		template <StringType stringType, typename... Args>
		void Write(String<stringType> const& str, Args&&... args)
		{
			Write(str.GetView(), std::forward<Args>(args)...);
		}

		template <StringType stringType, typename... Args>
		void Write(StringView<stringType> const& str, Args&&... args)
		{
			Write(natUtil::FormatString(str, std::forward<Args>(args)...));
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

		template <StringType stringType, typename... Args>
		void WriteLine(String<stringType> const& str, Args&&... args)
		{
			WriteLine(str.GetView(), std::forward<Args>(args)...);
		}

		template <StringType stringType, typename... Args>
		void WriteLine(StringView<stringType> const& str, Args&&... args)
		{
			WriteLine(natUtil::FormatString(str, std::forward<Args>(args)...));
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

		template <StringType stringType, typename... Args>
		void WriteErr(String<stringType> const& str, Args&&... args)
		{
			WriteErr(str.GetView(), std::forward<Args>(args)...);
		}

		template <StringType stringType, typename... Args>
		void WriteErr(StringView<stringType> const& str, Args&&... args)
		{
			WriteErr(natUtil::FormatString(str, std::forward<Args>(args)...));
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

		template <StringType stringType, typename... Args>
		void WriteLineErr(String<stringType> const& str, Args&&... args)
		{
			WriteLineErr(str.GetView(), std::forward<Args>(args)...);
		}

		template <StringType stringType, typename... Args>
		void WriteLineErr(StringView<stringType> const& str, Args&&... args)
		{
			WriteLineErr(natUtil::FormatString(str, std::forward<Args>(args)...));
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

#ifdef _WIN32
		Optional<WORD> m_DefaultColor;
#endif
	};
}
