#pragma once
#include "natConfig.h"
#include "natStream.h"
#include "natMisc.h"
#include <array>

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	加解密类型
	////////////////////////////////////////////////////////////////////////////////
	enum class CryptoType
	{
		None	= 0,	// 供可能的特殊加密类型使用

		Crypt	= 1,
		Decrypt	= 2,

		Mixed	= Crypt | Decrypt,
	};

	MAKE_ENUM_CLASS_BITMASK_TYPE(CryptoType);

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	加解密处理器
	////////////////////////////////////////////////////////////////////////////////
	struct ICryptoProcessor
		: natRefObj
	{
		~ICryptoProcessor();

		///	@brief	获得加解密类型
		virtual CryptoType GetCryptoType() const noexcept = 0;

		///	@brief	获得输入块大小
		///	@note	任何时候调用此方法都必须返回相同的值
		virtual nLen GetInputBlockSize() const noexcept = 0;

		///	@brief	获得输出块大小
		///	@note	任何时候调用此方法都必须返回相同的值
		virtual nLen GetOutputBlockSize() const noexcept = 0;

		///	@brief	是否可同时处理多个块
		virtual nBool CanProcessMultiBlocks() const noexcept = 0;

		///	@brief	是否可以重用处理器
		///	@note	指调用ProcessFinal后处理器可以重新初始化并继续下一次处理
		virtual nBool CanReuseProcessor() const noexcept = 0;

		///	@brief	处理下一段数据
		///	@param	inputData	输入数据
		///	@param	inputDataLength	输入数据的长度
		///	@param	outputData	输出数据
		///	@param	outputDataLength	输出数据的长度
		///	@return	已处理的输入数据的长度和输出数据的长度
		virtual std::pair<nLen, nLen> Process(ncData inputData, nLen inputDataLength, nData outputData, nLen outputDataLength) = 0;

		///	@brief	结束此次处理数据
		///	@param	inputData	输入数据
		///	@param	inputDataLength	输入数据的长度
		///	@return	已处理的输入数据的长度和输出的数据
		virtual std::pair<nLen, std::vector<nByte>> ProcessFinal(ncData inputData, nLen inputDataLength) = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	加解密算法
	////////////////////////////////////////////////////////////////////////////////
	struct ICryptoAlgorithm
		: natRefObj
	{
		~ICryptoAlgorithm();

		virtual natRefPointer<ICryptoProcessor> CreateEncryptor() = 0;
		virtual natRefPointer<ICryptoProcessor> CreateDecryptor() = 0;
	};

	// 似乎搞砸了，貌似需要先初始化头部再处理数据，先放着之后再改

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	PKzipWeak加解密算法处理器
	////////////////////////////////////////////////////////////////////////////////
	class PKzipWeakProcessor final
		: public natRefObjImpl<PKzipWeakProcessor, ICryptoProcessor>
	{
	public:
		enum : nLen
		{
			InputBlockSize = 1,
			OutputBlockSize = 1,
			HeaderSize = 12,
		};

		explicit PKzipWeakProcessor(CryptoType cryptoType);
		~PKzipWeakProcessor();

		void InitCipher(ncData password, nLen passwordLength);
		// 初始化头部之前需要初始化Keys
		// 解密时在处理之前必须初始化头部
		void InitHeaderFrom(ncData buffer, nLen bufferLength);
		void InitHeaderFrom(natRefPointer<natStream> const& stream);
		// 在处理已经结束后才可调用，会使Header失效
		nBool GetHeader(nData buffer, nLen bufferLength);
		nBool CheckHeaderWithCrc32(nuInt crc32) const;
		void GenerateHeaderWithCrc32(nuInt crc32);

		CryptoType GetCryptoType() const noexcept override;
		nLen GetInputBlockSize() const noexcept override;
		nLen GetOutputBlockSize() const noexcept override;
		nBool CanProcessMultiBlocks() const noexcept override;
		nBool CanReuseProcessor() const noexcept override;

		std::pair<nLen, nLen> Process(ncData inputData, nLen inputDataLength, nData outputData, nLen outputDataLength) override;

		// 结束本次处理，使Keys失效
		std::pair<nLen, std::vector<nByte>> ProcessFinal(ncData inputData, nLen inputDataLength) override;

	private:
		const nBool m_IsCrypt;
		Optional<std::array<nuInt, 3>> m_Keys;
		Optional<std::array<nByte, HeaderSize>> m_Header;

		void uncheckedProcess(ncData inputData, nLen inputDataLength, nData outputData);
		void decryptHeader();
	};

	class PKzipWeakAlgorithm final
		: public natRefObjImpl<PKzipWeakAlgorithm, ICryptoAlgorithm>
	{
	public:
		natRefPointer<ICryptoProcessor> CreateEncryptor() override;
		natRefPointer<ICryptoProcessor> CreateDecryptor() override;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	加解密流
	////////////////////////////////////////////////////////////////////////////////
	class natCryptoStream
		: public natRefObjImpl<natCryptoStream, natWrappedStream>
	{
	public:
		enum class CryptoStreamMode
		{
			Read,
			Write
		};

		natCryptoStream(natRefPointer<natStream> stream, natRefPointer<ICryptoProcessor> cryptoProcessor, CryptoStreamMode mode);
		~natCryptoStream();

		natRefPointer<ICryptoProcessor> GetProcessor() const noexcept;

		nBool CanWrite() const override;
		nBool CanRead() const override;
		nBool CanResize() const override;
		nBool CanSeek() const override;
		nLen GetSize() const override;
		void SetSize(nLen Size) override;
		nLen GetPosition() const override;
		void SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override;
		nLen ReadBytes(nData pData, nLen Length) override;
		nLen WriteBytes(ncData pData, nLen Length) override;

		void FlushFinalBlock();

	private:
		const natRefPointer<ICryptoProcessor> m_Processor;
		const size_t m_InputBlockSize, m_OutputBlockSize;
		const nBool m_IsReadMode;
		size_t m_InputBufferSize, m_OutputBufferSize;
		std::vector<nByte> m_InputBuffer, m_OutputBuffer;
		nBool m_FinalBlockProcessed;
	};
}
