////////////////////////////////////////////////////////////////////////////////
///	@file	natBinary.h
///	@brief	二进制读写工具类
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "natConfig.h"
#include "natStream.h"
#include "natEnvironment.h"

namespace NatsuLib
{
	namespace detail_
	{
		inline void SwapEndian(nData data, std::size_t size)
		{
			using std::swap;
			const auto maxI = size - 1;
			const auto swapCount = size / 2;
			for (std::size_t i = 0; i < swapCount; ++i)
			{
				swap(data[i], data[maxI - i]);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief		二进制读取类
	///	@note		读取类并以特定字节序作为特定POD类型解读
	///	@warning	不适用于多个成员的结构体
	////////////////////////////////////////////////////////////////////////////////
	class natBinaryReader
		: public natRefObjImpl<natBinaryReader, natRefObj>
	{
	public:
		explicit natBinaryReader(natRefPointer<natStream> stream, Environment::Endianness endianness = Environment::GetEndianness()) noexcept;
		~natBinaryReader();

		natRefPointer<natStream> GetUnderlyingStream() const noexcept;
		Environment::Endianness GetEndianness() const noexcept;

		void Skip(nLen bytes);

		///	@brief		读取流并作为POD类型T解读
		///	@warning	不适用于多个成员的结构体
		template <typename T>
		std::enable_if_t<std::is_pod<T>::value, T> ReadPod()
		{
			T ret;
			ReadPod(ret);

			return ret;
		}

		///	@brief		读取流并作为POD类型T解读
		///	@warning	不适用于多个成员的结构体
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

	////////////////////////////////////////////////////////////////////////////////
	///	@brief		二进制写入类
	///	@note		将特定POD类型以特定字节序写入流
	///	@warning	不适用于多个成员的结构体
	////////////////////////////////////////////////////////////////////////////////
	class natBinaryWriter
		: public natRefObjImpl<natBinaryWriter, natRefObj>
	{
	public:
		explicit natBinaryWriter(natRefPointer<natStream> stream, Environment::Endianness endianness = Environment::GetEndianness()) noexcept;
		~natBinaryWriter();

		natRefPointer<natStream> GetUnderlyingStream() const noexcept;
		Environment::Endianness GetEndianness() const noexcept;

		///	@brief		将POD类型的实例写入流
		///	@warning	不适用于多个成员的结构体
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
