#pragma once
#include "natConfig.h"
#include "natStream.h"
#include "natEnvironment.h"

namespace NatsuLib
{
	namespace detail_
	{
		inline void SwapEndian(nData data, size_t size)
		{
			using std::swap;
			const auto maxI = size - 1;
			const auto swapCount = size / 2;
			for (size_t i = 0; i < swapCount; ++i)
			{
				swap(data[i], data[maxI - i]);
			}
		}
	}

	class natBinaryReader
		: public natRefObjImpl<natRefObj>
	{
	public:
		explicit natBinaryReader(natRefPointer<natStream> stream, Environment::Endianness endianness = Environment::GetEndianness()) noexcept;
		~natBinaryReader();

		natRefPointer<natStream> GetUnderlyingStream() const noexcept;
		Environment::Endianness GetEndianness() const noexcept;

		void Skip(nLen bytes);

		// 不适用于多个成员的结构体
		template <typename T>
		std::enable_if_t<std::is_pod<T>::value, T> ReadPod()
		{
			T ret;
			ReadPod(ret);
			
			return ret;
		}

		// 不适用于多个成员的结构体
		template <typename T>
		std::enable_if_t<std::is_pod<T>::value> ReadPod(T& obj)
		{
			nLen readBytes;
			if ((readBytes = m_Stream->ReadBytes(reinterpret_cast<nData>(&obj), sizeof(T))) < sizeof(T))
			{
				nat_Throw(natException, "Only partial data ({0} bytes/{1} bytes requested) has been successfully read."_nv, readBytes, sizeof(T));
			}

			if (m_NeedSwapEndian)
			{
				detail_::SwapEndian(reinterpret_cast<nData>(&obj), sizeof(T));
			}
		}

	private:
		natRefPointer<natStream> m_Stream;
		const Environment::Endianness m_Endianness;
		const nBool m_NeedSwapEndian;
	};

	class natBinaryWriter
		: public natRefObjImpl<natRefObj>
	{
	public:
		explicit natBinaryWriter(natRefPointer<natStream> stream, Environment::Endianness endianness = Environment::GetEndianness()) noexcept;
		~natBinaryWriter();

		natRefPointer<natStream> GetUnderlyingStream() const noexcept;
		Environment::Endianness GetEndianness() const noexcept;

		// 不适用于多个成员的结构体
		template <typename T>
		std::enable_if_t<std::is_pod<T>::value> WritePod(T const& obj)
		{
			nLen writtenBytes;
			auto pRead = reinterpret_cast<ncData>(&obj);
			nByte buffer[sizeof(T)];
			if (m_NeedSwapEndian)
			{
#ifdef _MSC_VER
				const auto copyIterator = stdext::make_checked_array_iterator(buffer, std::size(buffer));
#else
				const auto copyIterator = buffer;
#endif
				std::reverse_copy(pRead, pRead + sizeof(T), copyIterator);
				pRead = buffer;
			}
			if ((writtenBytes = m_Stream->WriteBytes(pRead, sizeof(T))) < sizeof(T))
			{
				nat_Throw(natException, "Only partial data ({0} bytes/{1} bytes requested) has been successfully written."_nv, writtenBytes, sizeof(T));
			}
		}

	private:
		natRefPointer<natStream> m_Stream;
		const Environment::Endianness m_Endianness;
		const nBool m_NeedSwapEndian;
	};
}
