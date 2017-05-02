////////////////////////////////////////////////////////////////////////////////
///	@file	natCompression.h
///	@brief	ѹ��/��ѹ������
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "natConfig.h"
#include "natRefObj.h"
#include "natStream.h"
#include "natBinary.h"
#include "natMisc.h"
#include "natLinq.h"
#include "natCompressionStream.h"
#include "natCryptography.h"

namespace NatsuLib
{
	DeclareException(EntryEncryptedException, natException, "This entry is encrypted."_nv);
	DeclareException(EntryDecryptFailedException, natException, "Cannot decrypt entry with provided password."_nv);

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	Zipѹ���ĵ�
	///	@note	������л��棬����ṩ�������������������н��л���
	////////////////////////////////////////////////////////////////////////////////
	class natZipArchive
		: public natRefObjImpl<natZipArchive, natRefObj>
	{
	public:
		class ZipEntry;

		enum class ZipArchiveMode
		{
			Create,
			Read,
			Update
		};

		explicit natZipArchive(natRefPointer<natStream> stream, ZipArchiveMode mode = ZipArchiveMode::Read);
		natZipArchive(natRefPointer<natStream> stream, StringType encoding, ZipArchiveMode mode = ZipArchiveMode::Read);
		~natZipArchive();

		ZipArchiveMode GetOpenMode() const noexcept;

		///	@brief	���ض���������������
		natRefPointer<ZipEntry> CreateEntry(nStrView entryName);
		///	@brief	����������
		Linq<const natRefPointer<ZipEntry>> GetEntries() const;
		///	@brief	���ض���������������
		///	@note	��δ�ҵ��᷵��nullptr������ضԷ���ֵ���м��
		natRefPointer<ZipEntry> GetEntry(nStrView entryName) const;

	private:
		enum
		{
			FindingBufferSize = 32,
		};

		enum class ZipVersionNeeded : nuShort
		{
			Default = 10,
			ExplicitDirectory = 20,
			Deflate = 20,
			Deflate64 = 21,
			Zip64 = 45
		};

		enum class ZipVersionMadeByPlatform : nByte
		{
			Windows = 0,
			Unix = 3
		};

		static constexpr nuInt Mask32Bit = 0xFFFFFFFF;
		static constexpr nuShort Mask16Bit = 0xFFFF;

		natRefPointer<natStream> m_Stream;
		natRefPointer<natBinaryReader> m_Reader;
		natRefPointer<natBinaryWriter> m_Writer;

		std::unordered_map<nStrView, natRefPointer<ZipEntry>> m_EntriesMap;

		const StringType m_Encoding;
		const ZipArchiveMode m_Mode;

		void addEntry(natRefPointer<ZipEntry> entry);
		void close();
		void writeToFile();

		// ʵ����ʾ������ZipBlock���뱣֤Read�෽����ɺ����г�Ա���ѳ�ʼ��

		struct ExtraField
		{
			static constexpr size_t HeaderSize = 4;

			nuShort Tag;
			nuShort Size;
			std::vector<nByte> Data;

			void Read(natRefPointer<natBinaryReader> reader);
			nBool ReadWithLimit(natRefPointer<natBinaryReader> reader, nLen endExtraField);
			void Write(natRefPointer<natBinaryWriter> writer) const;

			size_t GetSize() const noexcept;
		};

		struct Zip64ExtraField
		{
			static constexpr size_t OffsetToFirstField = 4;
			static constexpr nuShort Tag = 1;

			nuShort Size;
			Optional<nuLong> UncompressedSize;
			Optional<nuLong> CompressedSize;
			Optional<nuLong> LocalHeaderOffset;
			Optional<nuInt> StartDiskNumber;

			nBool ReadFromExtraField(ExtraField const& extraField, nBool readUncompressedSize, nBool readCompressedSize, nBool readLocalHeaderOffset, nBool readStartDiskNumber);
			void Write(natRefPointer<natBinaryWriter> writer);
		};

		struct CentralDirectoryFileHeader
		{
			static constexpr nuInt Signature = 0x02014B50;

			nByte VersionMadeBySpecification;
			nByte VersionMadeByCompatibility;
			nuShort VersionNeededToExtract;
			nuShort GeneralPurposeBitFlag;
			nuShort CompressionMethod;
			nuInt LastModified;
			nuInt Crc32;
			nuLong CompressedSize;
			nuLong UncompressedSize;
			nuShort FilenameLength;
			nuShort ExtraFieldLength;
			nuShort FileCommentLength;
			nuInt DiskNumberStart;
			nuShort InternalFileAttributes;
			nuInt ExternalFileAttributes;
			nuLong RelativeOffsetOfLocalHeader;

			nString Filename;
			nString FileComment;
			std::vector<ExtraField> ExtraFields;

			nBool Read(natRefPointer<natBinaryReader> reader, nBool saveExtraFieldsAndComments, StringType encoding);
			void Write(natRefPointer<natBinaryWriter> writer, StringType encoding);
		};

		struct LocalFileHeader
		{
			static constexpr nuInt DataDescriptorSignature = 0x08074B50;
			static constexpr nuInt Signature = 0x04034B50;
			static constexpr size_t OffsetToCrcFromHeaderStart = 14;
			static constexpr size_t OffsetToFilenameLength = 26;
			static constexpr size_t SizeOfLocalHeader = 30;

			static nBool TrySkip(natRefPointer<natBinaryReader> reader);

			// ʵ����ʾ�����޸�header�е�FilenameLengthΪʵ��д����ļ�������
			static nBool Write(natRefPointer<natBinaryWriter> writer, CentralDirectoryFileHeader& header, Optional<std::deque<ExtraField>> const& localFileHeaderFields, StringType encoding);
			static void WriteCrcAndSizes(natRefPointer<natBinaryWriter> writer, CentralDirectoryFileHeader const& header, nBool usedZip64);
		};

		struct ZipEndOfCentralDirectory
		{
			static constexpr nuInt Signature = 0x06054B50;
			static constexpr size_t SizeOfBlockWithoutSignature = 18;

			nuShort NumberOfThisDisk;
			nuShort NumberOfTheDiskWithTheStartOfTheCentralDirectory;
			nuShort NumberOfEntriesInTheCentralDirectoryOnThisDisk;
			nuShort NumberOfEntriesInTheCentralDirectory;
			nuInt SizeOfTheCentralDirectory;
			nuInt OffsetOfStartOfCentralDirectoryWithRespectToTheStartingDiskNumber;
			nString ArchiveComment;

			void Read(natRefPointer<natBinaryReader> reader, StringType encoding);
			static void Write(natRefPointer<natBinaryWriter> writer, nuLong numberOfEntries, nuLong startOfCentralDirectory, nuLong sizeOfCentralDirectory, nStrView archiveComment, StringType encoding = nString::UsingStringType);
		};

		ZipEndOfCentralDirectory m_ZipEndOfCentralDirectory;

		struct Zip64EndOfCentralDirectoryLocator
		{
			static constexpr nuInt Signature = 0x07064B50;
			static constexpr size_t SizeOfBlockWithoutSignature = 16;

			nuInt NumberOfDiskWithZip64EOCD;
			nuLong OffsetOfZip64EOCD;
			nuInt TotalNumberOfDisks;

			void Read(natRefPointer<natBinaryReader> reader);
			static void Write(natRefPointer<natBinaryWriter> writer, nuLong zip64EOCDRecordStart);
		};

		Zip64EndOfCentralDirectoryLocator m_Zip64EndOfCentralDirectoryLocator;

		struct Zip64EndOfCentralDirectory
		{
			static constexpr nuInt Signature = 0x06064B50;
			static constexpr nuLong SizeWithoutExtraData = 0x2C;

			nuLong SizeOfThisRecord;
			nuShort VersionMadeBy;
			nuShort VersionNeededToExtract;
			nuInt NumberOfThisDisk;
			nuInt NumberOfDiskWithStartOfCD;
			nuLong NumberOfEntriesOnThisDisk;
			nuLong NumberOfEntriesTotal;
			nuLong SizeOfCentralDirectory;
			nuLong OffsetOfCentralDirectory;

			void Read(natRefPointer<natBinaryReader> reader);
			static void Write(natRefPointer<natBinaryWriter> writer, nuLong numberOfEntries, nuLong startOfCentralDirectory, nuLong sizeOfCentralDirectory);
		};

		Zip64EndOfCentralDirectory m_Zip64EndOfCentralDirectory;

		void internalOpen();
		void readCentralDirectory();
		void readEndOfCentralDirectory();

		void removeEntry(ZipEntry* entry);

		static nBool findSignatureBackward(natRefPointer<natStream> stream, nuInt signature);
		static std::pair<nBool, size_t> readStreamBackward(natRefPointer<natStream> stream, nData buffer, size_t bufferSize);

	public:
		////////////////////////////////////////////////////////////////////////////////
		///	@brief	Zipѹ���ĵ����
		////////////////////////////////////////////////////////////////////////////////
		class ZipEntry
			: public natRefObjImpl<ZipEntry, natRefObj>
		{
			friend class natZipArchive;
		public:
			~ZipEntry();

			enum class DecryptStatus
			{
				Success,
				Crc32CheckFailed,
				NotDecryptYet,
				NeedNotToDecrypt,
			};

			nStrView GetEntryName() const noexcept;
			nuLong GetCompressedSize() const noexcept;
			nuLong GetUncompressedSize() const noexcept;

			///	@brief	ɾ�����
			///	@note	ֻ���ڸ���ģʽ��ɾ����ڣ����������ڱ�д��ɾ������ʧ��
			void Delete();
			///	@brief	����ڲ�������
			natRefPointer<natStream> Open();

			void SetPassword();
			void SetPassword(ncData password, size_t passwordLength);
			void SetPassword(nStrView passwordStr);

			DecryptStatus GetDecryptStatus() const noexcept;

		private:
			enum class CompressionMethod : nuShort
			{
				Stored							= 0x0000,
				Shrunk							= 0x0001,
				ReducedWithCompressionFactor1	= 0x0002,
				ReducedWithCompressionFactor2	= 0x0003,
				ReducedWithCompressionFactor3	= 0x0004,
				ReducedWithCompressionFactor4	= 0x0005,
				Imploded						= 0x0006,
				/* ���� */
				Deflate							= 0x0008,
				Deflate64						= 0x0009,
				PKWareDCLImploded				= 0x000A,
				/* ���� */
				BZip2							= 0x000C,
				/* ���� */
				LZMA							= 0x000E,
				/* ���� */
				IBMTERSE						= 0x0012,
				IBMLZ77z						= 0x0013,
				PPMd							= 0x0062,
			};

			enum class BitFlag : nuShort
			{
				Encrypted				= 0x0001,
				CompressionOption1		= 0x0002,
				CompressionOption2		= 0x0004,
				DataDescriptor			= 0x0008,
				EnhancedDeflation		= 0x0010,
				CompressedPatchedData	= 0x0020,
				StrongEncryption		= 0x0040,
				/* δʹ�� */
				UnicodeFileName			= 0x0800,
				/* ���� */
				MaskHeaderValues		= 0x2000,
				/* ���� */
			};

			natZipArchive* m_Archive;
			const nBool m_OriginallyInArchive;
			CentralDirectoryFileHeader m_CentralDirectoryFileHeader;
			Optional<nLen> m_OffsetOfCompressedData;

			nBool m_EverOpenedForWrite, m_CurrentOpeningForWrite;

			Optional<std::deque<ExtraField>> m_LocalHeaderFields;

			Optional<std::vector<nByte>> m_Password;
			DecryptStatus m_DecryptStatus;

			// ����д�������
			natRefPointer<natStream> m_UncompressedData;
			// �ڸ���ģʽ�����δ����д�������ʱ����ԭ�ļ����ݣ���д��ģʽ������
			Optional<std::vector<nByte>> m_CachedCompressedData;

			natRefPointer<natCryptoStream> m_CryptoStream;

			ZipEntry(natZipArchive* archive, nStrView const& entryName);
			ZipEntry(natZipArchive* archive, CentralDirectoryFileHeader const& centralDirectoryFileHeader);

			natRefPointer<natStream> openForRead();
			natRefPointer<natStream> openForCreate();
			natRefPointer<natStream> openForUpdate();

			nLen getOffsetOfCompressedData();

			// ���δѹ�����ݣ����ڸ���ģʽʹ��
			natRefPointer<natStream> const& getUncompressedData();

			natRefPointer<natStream> createCompressor(natRefPointer<natStream> stream);

			void loadExtraFieldAndCompressedData();
			void writeLocalFileHeaderAndData();

			// �����ʱ�Ѿ�������m_CentralDirectoryFileHeader��Crc32Ϊ��ȷ��ֵ
			void writeSecurityMetadata(natRefPointer<natStream> const& stream);

			// �ȼ���Crc32����ѹ����������
			// ����ʱ����������Ȼ����Ҫ����Crc32��
			class ZipEntryWriteStream final
				: public natRefObjImpl<ZipEntryWriteStream, natWrappedStream>
			{
			public:
				ZipEntryWriteStream(ZipEntry& entry, natRefPointer<natStream> stream, std::function<void(ZipEntryWriteStream&)> finishCallback = {});
				~ZipEntryWriteStream();

				nLen GetInitialPosition() const noexcept;

				nBool CanWrite() const override;
				nBool CanRead() const override;
				nBool CanResize() const override;
				nBool CanSeek() const override;
				nBool IsEndOfStream() const override;
				nLen GetSize() const override;
				void SetSize(nLen /*Size*/) override;
				nLen GetPosition() const override;
				void SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override;
				nLen ReadBytes(nData pData, nLen Length) override;
				nLen WriteBytes(ncData pData, nLen Length) override;
				void Flush() override;

			private:
				ZipEntry& m_Entry;
				nLen m_InitialPosition;
				nBool m_WroteData, m_UseZip64;
				std::function<void(ZipEntryWriteStream&)> m_FinishCallback;

				void finish();
			};
		};
	};
}
