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
	///	@brief	压缩文档
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

		natRefPointer<ZipEntry> CreateEntry(nStrView entryName);
		Linq<const natRefPointer<ZipEntry>> GetEntries() const;
		natRefPointer<ZipEntry> GetEntry(nStrView entryName) const;

	private:
		enum
		{
			FindingBufferSize = 32,
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

		struct ExtraField
		{
			static constexpr size_t HeaderSize = 4;

			nuShort Tag;
			nuShort Size;
			std::vector<nByte> Data;

			void Read(natBinaryReader* reader);
			nBool ReadWithLimit(natBinaryReader* reader, nLen endExtraField);
			void Write(natBinaryWriter* writer);

			size_t GetSize() const noexcept;
		};

		struct Zip64ExtraField
		{
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

			nByte VersionMadeByCompatibility;
			nByte VersionMadeBySpecification;
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
			void Write(natBinaryWriter* writer);
		};

		struct LocalFileHeader
		{
			static constexpr nuInt DataDescriptorSignature = 0x08074B50;
			static constexpr nuInt Signature = 0x04034B50;

			static nBool TrySkip(natBinaryReader* reader);
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
			void Write(natBinaryWriter* writer);
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
			void Write(natBinaryWriter* writer);
		};

		Zip64EndOfCentralDirectoryLocator m_Zip64EndOfCentralDirectoryLocator;

		struct Zip64EndOfCentralDirectory
		{
			static constexpr nuInt Signature = 0x06064B50;
			static constexpr nuLong SizeWithoutExtraFields = 0x2C;

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
			void Write(natBinaryWriter* writer);
		};

		Zip64EndOfCentralDirectory m_Zip64EndOfCentralDirectory;

		void internalOpen();
		void readCentralDirectory();
		void readEndOfCentralDirectory();

		void removeEntry(ZipEntry* entry);

		static nBool findSignatureBackward(natStream* stream, nuInt signature);
		static std::pair<nBool, size_t> readStreamBackward(natStream* stream, nData buffer, size_t bufferSize);

	public:
		class ZipEntry
			: public natRefObjImpl<natRefObj>
		{
			friend class natZipArchive;
		public:
			ZipEntry(natZipArchive* archive, nStrView const& entryName);
			~ZipEntry();

			void Delete();
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

			natZipArchive* m_Archive;
			CentralDirectoryFileHeader m_CentralDirectoryFileHeader;
			Optional<nLen> m_OffsetOfCompressedData;
			natRefPointer<natStream> m_Stream;

			ZipEntry(natZipArchive* archive, CentralDirectoryFileHeader const& centralDirectoryFileHeader);

			void openInReadMode();
			void openInCreateMode();
			void openInUpdateMode();
		};
	};
}
