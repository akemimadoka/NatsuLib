////////////////////////////////////////////////////////////////////////////////
///	@file	natCompression.h
///	@brief	压缩/解压工具类
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "natConfig.h"
#include "natRefObj.h"
#include "natStream.h"
#include "natBinary.h"
#include "natMisc.h"
#include "natLinq.h"
#include "natCompressionStream.h"

namespace NatsuLib
{
	
	////////////////////////////////////////////////////////////////////////////////
	///	@brief	Zip压缩文档
	///	@note	不会进行缓存，如果提供的流不符合条件请自行进行缓存
	////////////////////////////////////////////////////////////////////////////////
	class natZipArchive
		: public natRefObjImpl<natRefObj>
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

		///	@brief	以特定的入口名创建入口
		natRefPointer<ZipEntry> CreateEntry(nStrView entryName);
		///	@brief	获得所有入口
		Linq<const natRefPointer<ZipEntry>> GetEntries() const;
		///	@brief	以特定的入口名查找入口
		///	@note	若未找到会返回nullptr，请务必对返回值进行检查
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

		// 实现提示：所有ZipBlock必须保证Read类方法完成后所有成员都已初始化

		struct ExtraField
		{
			static constexpr size_t HeaderSize = 4;

			nuShort Tag;
			nuShort Size;
			std::vector<nByte> Data;

			void Read(natBinaryReader* reader);
			nBool ReadWithLimit(natBinaryReader* reader, nLen endExtraField);
			void Write(natBinaryWriter* writer) const;

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
			void Write(natBinaryWriter* writer);
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

			nBool Read(natBinaryReader* reader, nBool saveExtraFieldsAndComments, StringType encoding);
			void Write(natBinaryWriter* writer, StringType encoding);
		};

		struct LocalFileHeader
		{
			static constexpr nuInt DataDescriptorSignature = 0x08074B50;
			static constexpr nuInt Signature = 0x04034B50;
			static constexpr size_t OffsetToCrcFromHeaderStart = 14;
			static constexpr size_t OffsetToFilenameLength = 26;
			static constexpr size_t SizeOfLocalHeader = 30;

			static nBool TrySkip(natBinaryReader* reader);

			// 实现提示：会修改header中的FilenameLength为实际写入的文件名长度
			static nBool Write(natBinaryWriter* writer, CentralDirectoryFileHeader& header, Optional<std::deque<ExtraField>> const& localFileHeaderFields, StringType encoding);
			static void WriteCrcAndSizes(natBinaryWriter* writer, CentralDirectoryFileHeader const& header, nBool usedZip64);
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

			void Read(natBinaryReader* reader, StringType encoding);
			static void Write(natBinaryWriter* writer, nuLong numberOfEntries, nuLong startOfCentralDirectory, nuLong sizeOfCentralDirectory, nStrView archiveComment, StringType encoding = nString::UsingStringType);
		};

		ZipEndOfCentralDirectory m_ZipEndOfCentralDirectory;

		struct Zip64EndOfCentralDirectoryLocator
		{
			static constexpr nuInt Signature = 0x07064B50;
			static constexpr size_t SizeOfBlockWithoutSignature = 16;

			nuInt NumberOfDiskWithZip64EOCD;
			nuLong OffsetOfZip64EOCD;
			nuInt TotalNumberOfDisks;

			void Read(natBinaryReader* reader);
			static void Write(natBinaryWriter* writer, nuLong zip64EOCDRecordStart);
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

			void Read(natBinaryReader* reader);
			static void Write(natBinaryWriter* writer, nuLong numberOfEntries, nuLong startOfCentralDirectory, nuLong sizeOfCentralDirectory);
		};

		Zip64EndOfCentralDirectory m_Zip64EndOfCentralDirectory;

		void internalOpen();
		void readCentralDirectory();
		void readEndOfCentralDirectory();

		void removeEntry(ZipEntry* entry);

		static nBool findSignatureBackward(natStream* stream, nuInt signature);
		static std::pair<nBool, size_t> readStreamBackward(natStream* stream, nData buffer, size_t bufferSize);

	public:
		////////////////////////////////////////////////////////////////////////////////
		///	@brief	Zip压缩文档入口
		////////////////////////////////////////////////////////////////////////////////
		class ZipEntry
			: public natRefObjImpl<natRefObj>
		{
			friend class natZipArchive;
		public:
			~ZipEntry();

			///	@brief	删除入口
			///	@note	只能在更新模式下删除入口，如果入口正在被写入删除将会失败
			void Delete();
			///	@brief	打开入口并返回流
			natRefPointer<natStream> Open();

		private:
			enum class CompressionMethod : nuShort
			{
				Stored = 0x0,
				Deflate = 0x8,
				Deflate64 = 0x9,
				BZip2 = 0xC,
				LZMA = 0xE
			};

			enum class BitFlag : nuShort
			{
				DataDescriptor = 0x8,
				UnicodeFileName = 0x800
			};

			natZipArchive* m_Archive;
			const nBool m_OriginallyInArchive;
			CentralDirectoryFileHeader m_CentralDirectoryFileHeader;
			Optional<nLen> m_OffsetOfCompressedData;

			nBool m_EverOpenedForWrite, m_CurrentOpeningForWrite;

			Optional<std::deque<ExtraField>> m_LocalHeaderFields;

			// 缓存写入的数据
			natRefPointer<natStream> m_UncompressedData;
			// 在更新模式且入口未由于写入而加载时缓存原文件内容，在写入模式无作用
			Optional<std::vector<nByte>> m_CachedCompressedData;

			ZipEntry(natZipArchive* archive, nStrView const& entryName);
			ZipEntry(natZipArchive* archive, CentralDirectoryFileHeader const& centralDirectoryFileHeader);

			natRefPointer<natStream> openForRead();
			natRefPointer<natStream> openForCreate();
			natRefPointer<natStream> openForUpdate();

			nLen getOffsetOfCompressedData();

			natRefPointer<natStream> const& getUncompressedData();

			natRefPointer<natStream> createCompressor(natRefPointer<natStream> stream);

			void loadExtraFieldAndCompressedData();
			void writeLocalFileHeaderAndData();

			class ZipEntryWriteStream final
				: public natRefObjImpl<natStream>
			{
			public:
				ZipEntryWriteStream(ZipEntry& entry, natRefPointer<natCrc32Stream> crc32Stream, std::function<void()> finishCallback = {});
				~ZipEntryWriteStream();

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
				natRefPointer<natCrc32Stream> m_InternalStream;
				nLen m_InitialPosition;
				nBool m_WroteData, m_UseZip64;
				std::function<void()> m_FinishCallback;

				void finish();
			};

			class DisposeCallbackStream final
				: public natRefObjImpl<natStream>
			{
			public:
				explicit DisposeCallbackStream(natRefPointer<natStream> internalStream, std::function<void()> disposeCallback = {});
				~DisposeCallbackStream();

				nBool HasDisposeCallback() const noexcept;
				natRefPointer<natStream> GetUnderlyingStream() const noexcept;

				nBool CanWrite() const override;
				nBool CanRead() const override;
				nBool CanResize() const override;
				nBool CanSeek() const override;
				nBool IsEndOfStream() const override;
				nLen GetSize() const override;
				void SetSize(nLen Size) override;
				nLen GetPosition() const override;
				void SetPosition(NatSeek Origin, nLong Offset) override;
				nByte ReadByte() override;
				nLen ReadBytes(nData pData, nLen Length) override;
				std::future<nLen> ReadBytesAsync(nData pData, nLen Length) override;
				void WriteByte(nByte byte) override;
				nLen WriteBytes(ncData pData, nLen Length) override;
				std::future<nLen> WriteBytesAsync(ncData pData, nLen Length) override;
				void Flush() override;

			private:
				natRefPointer<natStream> m_InternalStream;
				std::function<void()> m_DisposeCallback;
			};
		};
	};
}
