#pragma once
#include "natConfig.h"
#include "natStream.h"
#include "natMisc.h"
#include <array>

namespace NatsuLib
{
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	�ӽ�������
	////////////////////////////////////////////////////////////////////////////////
	enum class CryptoType
	{
		None	= 0,	// �����ܵ������������ʹ��

		Crypt	= 1,
		Decrypt	= 2,

		Mixed	= Crypt | Decrypt,
	};

	MAKE_ENUM_CLASS_BITMASK_TYPE(CryptoType);

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	�ӽ��ܴ�����
	////////////////////////////////////////////////////////////////////////////////
	struct ICryptoProcessor
		: natRefObj
	{
		~ICryptoProcessor();

		///	@brief	��üӽ�������
		virtual CryptoType GetCryptoType() const noexcept = 0;

		///	@brief	���������С
		///	@note	�κ�ʱ����ô˷��������뷵����ͬ��ֵ
		virtual nLen GetInputBlockSize() const noexcept = 0;

		///	@brief	���������С
		///	@note	�κ�ʱ����ô˷��������뷵����ͬ��ֵ
		virtual nLen GetOutputBlockSize() const noexcept = 0;

		///	@brief	�Ƿ��ͬʱ��������
		virtual nBool CanProcessMultiBlocks() const noexcept = 0;

		///	@brief	�Ƿ�������ô�����
		///	@note	ָ����ProcessFinal�������������³�ʼ����������һ�δ���
		virtual nBool CanReuseProcessor() const noexcept = 0;

		///	@brief	������һ������
		///	@param	inputData	��������
		///	@param	inputDataLength	�������ݵĳ���
		///	@param	outputData	�������
		///	@param	outputDataLength	������ݵĳ���
		///	@return	�Ѵ�����������ݵĳ��Ⱥ�������ݵĳ���
		virtual std::pair<nLen, nLen> Process(ncData inputData, nLen inputDataLength, nData outputData, nLen outputDataLength) = 0;

		///	@brief	�����˴δ�������
		///	@param	inputData	��������
		///	@param	inputDataLength	�������ݵĳ���
		///	@return	�Ѵ�����������ݵĳ��Ⱥ����������
		virtual std::pair<nLen, std::vector<nByte>> ProcessFinal(ncData inputData, nLen inputDataLength) = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	�ӽ����㷨
	////////////////////////////////////////////////////////////////////////////////
	struct ICryptoAlgorithm
		: natRefObj
	{
		~ICryptoAlgorithm();

		virtual natRefPointer<ICryptoProcessor> CreateEncryptor() = 0;
		virtual natRefPointer<ICryptoProcessor> CreateDecryptor() = 0;
	};

	// �ƺ������ˣ�ò����Ҫ�ȳ�ʼ��ͷ���ٴ������ݣ��ȷ���֮���ٸ�

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	PKzipWeak�ӽ����㷨������
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
		// ��ʼ��ͷ��֮ǰ��Ҫ��ʼ��Keys
		// ����ʱ�ڴ���֮ǰ�����ʼ��ͷ��
		void InitHeaderFrom(ncData buffer, nLen bufferLength);
		void InitHeaderFrom(natRefPointer<natStream> const& stream);
		// �ڴ����Ѿ�������ſɵ��ã���ʹHeaderʧЧ
		nBool GetHeader(nData buffer, nLen bufferLength);
		nBool CheckHeaderWithCrc32(nuInt crc32) const;
		void GenerateHeaderWithCrc32(nuInt crc32);

		CryptoType GetCryptoType() const noexcept override;
		nLen GetInputBlockSize() const noexcept override;
		nLen GetOutputBlockSize() const noexcept override;
		nBool CanProcessMultiBlocks() const noexcept override;
		nBool CanReuseProcessor() const noexcept override;

		std::pair<nLen, nLen> Process(ncData inputData, nLen inputDataLength, nData outputData, nLen outputDataLength) override;

		// �������δ���ʹKeysʧЧ
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
	///	@brief	�ӽ�����
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
