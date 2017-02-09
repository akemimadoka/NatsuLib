#include "stdafx.h"
#include "natCompression.h"
#include "natEncoding.h"

using namespace NatsuLib;

natZipArchive::ZipEntry::~ZipEntry()
{
}

void natZipArchive::ZipEntry::Delete()
{
	if (m_Archive)
	{
		m_Archive->removeEntry(this);
		m_Archive = nullptr;
	}
}

natRefPointer<natStream> natZipArchive::ZipEntry::Open()
{
	openInReadMode();
	return m_Stream;
}

natZipArchive::ZipEntry::ZipEntry(natZipArchive* archive, CentralDirectoryFileHeader const& centralDirectoryFileHeader)
	: m_Archive{ archive }, m_CentralDirectoryFileHeader(centralDirectoryFileHeader)
{
}

void natZipArchive::ZipEntry::openInReadMode()
{
	if (m_Stream)
	{
		return;
	}

	if (!m_OffsetOfCompressedData)
	{
		const auto localHeaderOffset = m_CentralDirectoryFileHeader.RelativeOffsetOfLocalHeader;
		m_Archive->m_Stream->SetPosition(NatSeek::Beg, localHeaderOffset);
		if (!LocalFileHeader::TrySkip(m_Archive->m_Reader))
		{
			nat_Throw(InvalidData, "Invalid data."_nv);
		}
		m_OffsetOfCompressedData = m_Archive->m_Stream->GetPosition();
	}

	const auto offset = m_OffsetOfCompressedData.value();
	auto compressedStream = make_ref<natSubStream>(m_Archive->m_Stream, offset, offset + m_CentralDirectoryFileHeader.CompressedSize);
	switch (static_cast<CompressionMethod>(m_CentralDirectoryFileHeader.CompressionMethod))
	{
	case CompressionMethod::Deflate:
		m_Stream = make_ref<natDeflateStream>(std::move(compressedStream));
		break;
	case CompressionMethod::Deflate64:
	case CompressionMethod::BZip2:
	case CompressionMethod::LZMA:
		nat_Throw(natErrException, NatErr_NotImpl, "This compress method has not implemented yet."_nv);
	case CompressionMethod::Stored:
	default:
		m_Stream = std::move(compressedStream);
	}
}

natZipArchive::ZipEntry::ZipEntry(natZipArchive* archive, nStrView const& entryName)
	: m_Archive{ archive }, m_CentralDirectoryFileHeader{}, m_OffsetOfCompressedData{}
{
	m_CentralDirectoryFileHeader.Filename = entryName;
	m_CentralDirectoryFileHeader.FilenameLength = static_cast<nuShort>(m_CentralDirectoryFileHeader.Filename.size() * sizeof(StringEncodingTrait<nString::UsingStringType>::CharType));
}

natZipArchive::natZipArchive(natRefPointer<natStream> stream, ZipArchiveMode mode)
#ifdef _WIN32
	: natZipArchive(std::move(stream), StringType::Ansi, mode)
#else
	: natZipArchive(std::move(stream), StringType::Utf8, mode)
#endif
{
}

natZipArchive::natZipArchive(natRefPointer<natStream> stream, StringType encoding, ZipArchiveMode mode)
	: m_Stream{ std::move(stream) }, m_Reader{ make_ref<natBinaryReader>(m_Stream, Environment::Endianness::LittleEndian) }, m_Encoding{ encoding }, m_Mode{ mode }
{
	switch (mode)
	{
	case ZipArchiveMode::Create:
		if (!m_Stream->CanWrite() || !m_Stream->CanSeek())
		{
			nat_Throw(natErrException, NatErr_InvalidArg, "stream should be writable and seekable with ZipArchiveMode::Create mode."_nv);
		}
		m_Writer = make_ref<natBinaryWriter>(m_Stream, Environment::Endianness::LittleEndian);
		break;
	case ZipArchiveMode::Read:
		if (!m_Stream->CanRead() || !m_Stream->CanSeek())
		{
			nat_Throw(natErrException, NatErr_InvalidArg, "stream should be readable and seekable with ZipArchiveMode::Read mode."_nv);
		}
		m_Reader = make_ref<natBinaryReader>(m_Stream, Environment::Endianness::LittleEndian);
		break;
	case ZipArchiveMode::Update:
		if (!m_Stream->CanRead() || !m_Stream->CanWrite() || !m_Stream->CanSeek())
		{
			nat_Throw(natErrException, NatErr_InvalidArg, "stream should be readable, writable and seekable with ZipArchiveMode::Update mode."_nv);
		}
		m_Reader = make_ref<natBinaryReader>(m_Stream, Environment::Endianness::LittleEndian);
		m_Writer = make_ref<natBinaryWriter>(m_Stream, Environment::Endianness::LittleEndian);
		break;
	default:
		assert(!"Invalid mode value.");
		break;
	}
	
	internalOpen();
}

natZipArchive::~natZipArchive()
{
}

natRefPointer<natZipArchive::ZipEntry> natZipArchive::CreateEntry(nStrView entryName)
{
	auto entry = make_ref<ZipEntry>(this, entryName);
	m_EntriesMap.emplace(entry->m_CentralDirectoryFileHeader.Filename, entry);
	return std::move(entry);
}

Linq<const natRefPointer<natZipArchive::ZipEntry>> natZipArchive::GetEntries() const
{
	return from(m_EntriesMap).select([](auto&& pair) -> const natRefPointer<ZipEntry>& { return pair.second; });
}

natRefPointer<natZipArchive::ZipEntry> natZipArchive::GetEntry(nStrView entryName) const
{
	const auto iter = m_EntriesMap.find(entryName);
	if (iter == m_EntriesMap.end())
	{
		nat_Throw(natErrException, NatErr_NotFound, "No such entry."_nv);
	}
	return iter->second;
}

void natZipArchive::addEntry(natRefPointer<ZipEntry> entry)
{
	m_EntriesMap.emplace(entry->m_CentralDirectoryFileHeader.Filename, std::move(entry));
}

void natZipArchive::ExtraField::Read(natBinaryReader* reader)
{
	Tag = reader->ReadPod<nuShort>();
	Size = reader->ReadPod<nuShort>();
	Data.resize(Size);
	reader->GetUnderlyingStream()->ReadBytes(Data.data(), Size);
}

nBool natZipArchive::ExtraField::ReadWithLimit(natBinaryReader* reader, nLen endExtraField)
{
	const auto stream = reader->GetUnderlyingStream();
	if (endExtraField - stream->GetPosition() < 4)
	{
		return false;
	}

	Tag = reader->ReadPod<nuShort>();
	Size = reader->ReadPod<nuShort>();

	if (endExtraField - stream->GetPosition() < Size)
	{
		return false;
	}

	Data.resize(Size);
	stream->ReadBytes(Data.data(), Size);

	return true;
}

void natZipArchive::ExtraField::Write(natBinaryWriter* writer)
{
}

size_t natZipArchive::ExtraField::GetSize() const noexcept
{
	return Size + HeaderSize;
}

nBool natZipArchive::Zip64ExtraField::ReadFromExtraField(ExtraField const& extraField, nBool readUncompressedSize, nBool readCompressedSize, nBool readLocalHeaderOffset, nBool readStartDiskNumber)
{
	if (extraField.Tag != Tag)
	{
		return false;
	}

	natBinaryReader reader{ natMemoryStream::CreateFromExternMemory(extraField.Data.data(), extraField.Data.size(), true),
		Environment::Endianness::LittleEndian };

	Size = extraField.Size;

	nuShort expectedSize = 0;

	if (readUncompressedSize)
	{
		expectedSize += 8;
	}
	if (readCompressedSize)
	{
		expectedSize += 8;
	}
	if (readLocalHeaderOffset)
	{
		expectedSize += 8;
	}
	if (readStartDiskNumber)
	{
		expectedSize += 4;
	}

	if (expectedSize != Size)
	{
		return false;
	}

	if (readUncompressedSize)
	{
		UncompressedSize = reader.ReadPod<nuLong>();
		if (UncompressedSize.value() > static_cast<nuLong>(std::numeric_limits<nLong>::max()))
		{
			nat_Throw(InvalidData, "UncompressedSize is too big."_nv);
		}
	}

	if (readCompressedSize)
	{
		CompressedSize = reader.ReadPod<nuLong>();
		if (CompressedSize.value() > static_cast<nuLong>(std::numeric_limits<nLong>::max()))
		{
			nat_Throw(InvalidData, "CompressedSize is too big."_nv);
		}
	}

	if (readLocalHeaderOffset)
	{
		LocalHeaderOffset = reader.ReadPod<nuLong>();
		if (LocalHeaderOffset.value() > static_cast<nuLong>(std::numeric_limits<nLong>::max()))
		{
			nat_Throw(InvalidData, "LocalHeaderOffset is too big."_nv);
		}
	}

	if (readStartDiskNumber)
	{
		StartDiskNumber = reader.ReadPod<nuInt>();
		if (StartDiskNumber.value() > static_cast<nuLong>(std::numeric_limits<nLong>::max()))
		{
			nat_Throw(InvalidData, "StartDiskNumber is too big."_nv);
		}
	}

	return true;
}

void natZipArchive::Zip64ExtraField::Write(natBinaryWriter* writer)
{
}

nBool natZipArchive::CentralDirectoryFileHeader::Read(natBinaryReader* reader, nBool saveExtraFieldsAndComments, StringType encoding)
{
	if (reader->ReadPod<nuInt>() != Signature)
	{
		return false;
	}

	const auto stream = reader->GetUnderlyingStream();

	VersionMadeBySpecification = reader->ReadPod<nByte>();
	VersionMadeByCompatibility = reader->ReadPod<nByte>();
	VersionNeededToExtract = reader->ReadPod<nuShort>();
	GeneralPurposeBitFlag = reader->ReadPod<nuShort>();
	CompressionMethod = reader->ReadPod<nuShort>();
	LastModified = reader->ReadPod<nuInt>();
	Crc32 = reader->ReadPod<nuInt>();
	const auto compressedSizeSmall = reader->ReadPod<nuInt>();
	const auto uncompressedSizeSmall = reader->ReadPod<nuInt>();
	FilenameLength = reader->ReadPod<nuShort>();
	ExtraFieldLength = reader->ReadPod<nuShort>();
	FileCommentLength = reader->ReadPod<nuShort>();
	const auto diskNumberStartSmall = reader->ReadPod<nuShort>();
	InternalFileAttributes = reader->ReadPod<nuShort>();
	ExternalFileAttributes = reader->ReadPod<nuInt>();
	const auto relativeOffsetOfLocalHeaderSmall = reader->ReadPod<nuInt>();

	if (FilenameLength > 0)
	{
		if (encoding == nString::UsingStringType)
		{
			Filename.Resize(FilenameLength + 1);
			stream->ReadBytes(reinterpret_cast<nData>(Filename.data()), FilenameLength);
		}
		else
		{
			std::vector<nByte> buffer(FilenameLength);
			stream->ReadBytes(buffer.data(), FilenameLength);
			Filename.Assign(RuntimeEncoding<nString::UsingStringType>::Encode(buffer.data(), FilenameLength, encoding));
		}
	}

	nBool uncompressedSizeInZip64 = uncompressedSizeSmall == Mask32Bit;
	nBool compressedSizeInZip64 = compressedSizeSmall == Mask32Bit;
	nBool relativeOffsetInZip64 = relativeOffsetOfLocalHeaderSmall == Mask32Bit;
	nBool diskNumberStartInZip64 = diskNumberStartSmall == Mask16Bit;

	const auto endPosition = stream->GetPosition() + ExtraFieldLength;

	Zip64ExtraField zip64ExtraField;
	ExtraField extraField;
	if (saveExtraFieldsAndComments)
	{
		auto zip64FieldFound = false;
		while (extraField.ReadWithLimit(reader, endPosition))
		{
			if (extraField.Tag == Zip64ExtraField::Tag)
			{
				if (!zip64FieldFound)
				{
					zip64ExtraField.ReadFromExtraField(extraField, uncompressedSizeInZip64, compressedSizeInZip64, relativeOffsetInZip64, diskNumberStartInZip64);
					zip64FieldFound = true;
				}
			}
			else
			{
				ExtraFields.emplace_back(extraField);
			}
		}
	}
	else
	{
		while (extraField.ReadWithLimit(reader, endPosition) && !zip64ExtraField.ReadFromExtraField(extraField, uncompressedSizeInZip64, compressedSizeInZip64, relativeOffsetInZip64, diskNumberStartInZip64))
		{
		}
	}

	stream->SetPosition(NatSeek::Beg, endPosition);

	if (FileCommentLength > 0)
	{
		if (saveExtraFieldsAndComments)
		{
			if (encoding == nString::UsingStringType)
			{
				FileComment.Resize(FileCommentLength + 1);
				stream->ReadBytes(reinterpret_cast<nData>(FileComment.data()), FileCommentLength);
			}
			else
			{
				std::vector<nByte> buffer(FileCommentLength);
				stream->ReadBytes(buffer.data(), FileCommentLength);
				FileComment.Assign(RuntimeEncoding<nString::UsingStringType>::Encode(buffer.data(), FileCommentLength, encoding));
			}
		}
		else
		{
			stream->SetPosition(NatSeek::Cur, FileCommentLength);
		}
	}

	UncompressedSize = zip64ExtraField.UncompressedSize.value_or(uncompressedSizeSmall);
	CompressedSize = zip64ExtraField.CompressedSize.value_or(compressedSizeSmall);
	RelativeOffsetOfLocalHeader = zip64ExtraField.LocalHeaderOffset.value_or(relativeOffsetOfLocalHeaderSmall);
	DiskNumberStart = zip64ExtraField.StartDiskNumber.value_or(diskNumberStartSmall);

	return true;
}

void natZipArchive::CentralDirectoryFileHeader::Write(natBinaryWriter* writer)
{
}

nBool natZipArchive::LocalFileHeader::TrySkip(natBinaryReader* reader)
{
	constexpr nLong OffsetToFilenameLength = 22;

	const auto stream = reader->GetUnderlyingStream();

	if (reader->ReadPod<nuInt>() != Signature)
	{
		return false;
	}

	if (stream->GetSize() < stream->GetPosition() + OffsetToFilenameLength)
	{
		return false;
	}

	stream->SetPosition(NatSeek::Cur, OffsetToFilenameLength);

	const auto filenameLength = reader->ReadPod<nuShort>();
	const auto extraFieldLength = reader->ReadPod<nuShort>();

	if (stream->GetSize() < stream->GetPosition() + filenameLength + extraFieldLength)
	{
		return false;
	}

	stream->SetPosition(NatSeek::Cur, filenameLength + extraFieldLength);

	return true;
}

void natZipArchive::ZipEndOfCentralDirectory::Read(natBinaryReader* reader, StringType encoding)
{
	const auto stream = reader->GetUnderlyingStream();
#ifdef _DEBUG
	if (reader->ReadPod<nuInt>() != Signature)
	{
		assert(!"We are not reading valid eocd block.");
	}
#else
	reader->Skip(sizeof(nuInt));
#endif
	NumberOfThisDisk = reader->ReadPod<nuShort>();
	NumberOfTheDiskWithTheStartOfTheCentralDirectory = reader->ReadPod<nuShort>();
	NumberOfEntriesInTheCentralDirectoryOnThisDisk = reader->ReadPod<nuShort>();
	NumberOfEntriesInTheCentralDirectory = reader->ReadPod<nuShort>();

	if (NumberOfEntriesInTheCentralDirectory != NumberOfEntriesInTheCentralDirectoryOnThisDisk)
	{
		nat_Throw(InvalidData, "NumberOfEntriesInTheCentralDirectory does not equal to NumberOfEntriesInTheCentralDirectoryOnThisDisk."_nv);
	}

	SizeOfTheCentralDirectory = reader->ReadPod<nuInt>();
	OffsetOfStartOfCentralDirectoryWithRespectToTheStartingDiskNumber = reader->ReadPod<nuInt>();

	const auto commentSize = reader->ReadPod<nuShort>();

	if (commentSize > 0)
	{
		if (encoding == nString::UsingStringType)
		{
			ArchiveComment.Resize(commentSize + 1);
			stream->ReadBytes(reinterpret_cast<nData>(ArchiveComment.data()), commentSize);
		}
		else
		{
			std::vector<nByte> buffer(commentSize);
			stream->ReadBytes(buffer.data(), commentSize);
			ArchiveComment.Assign(RuntimeEncoding<nString::UsingStringType>::Encode(buffer.data(), commentSize, encoding));
		}
	}
}

void natZipArchive::ZipEndOfCentralDirectory::Write(natBinaryWriter* writer)
{
}

void natZipArchive::Zip64EndOfCentralDirectoryLocator::Read(natBinaryReader* reader)
{
#ifdef _DEBUG
	if (reader->ReadPod<nuInt>() != Signature)
	{
		assert(!"We are not reading valid eocd locator block.");
	}
#else
	reader->Skip(sizeof(nuInt));
#endif
	NumberOfDiskWithZip64EOCD = reader->ReadPod<nuInt>();
	OffsetOfZip64EOCD = reader->ReadPod<nuLong>();
	TotalNumberOfDisks = reader->ReadPod<nuInt>();
}

void natZipArchive::Zip64EndOfCentralDirectoryLocator::Write(natBinaryWriter* writer)
{
}

void natZipArchive::Zip64EndOfCentralDirectory::Read(natBinaryReader* reader)
{
	if (reader->ReadPod<nuInt>() != Signature)
	{
		nat_Throw(InvalidData, "Zip64EndOfCentralDirectory is invalid."_nv);
	}

	SizeOfThisRecord = reader->ReadPod<nuLong>();
	VersionMadeBy = reader->ReadPod<nuShort>();
	VersionNeededToExtract = reader->ReadPod<nuShort>();
	NumberOfThisDisk = reader->ReadPod<nuInt>();
	NumberOfDiskWithStartOfCD = reader->ReadPod<nuInt>();
	NumberOfEntriesOnThisDisk = reader->ReadPod<nuLong>();
	NumberOfEntriesTotal = reader->ReadPod<nuLong>();
	SizeOfCentralDirectory = reader->ReadPod<nuLong>();
	OffsetOfCentralDirectory = reader->ReadPod<nuLong>();
}

void natZipArchive::Zip64EndOfCentralDirectory::Write(natBinaryWriter* writer)
{
}

void natZipArchive::internalOpen()
{
	readEndOfCentralDirectory();
	readCentralDirectory();
}

void natZipArchive::readCentralDirectory()
{
	m_Stream->SetPosition(NatSeek::Beg, m_Zip64EndOfCentralDirectory.OffsetOfCentralDirectory ? m_Zip64EndOfCentralDirectory.OffsetOfCentralDirectory : m_ZipEndOfCentralDirectory.OffsetOfStartOfCentralDirectoryWithRespectToTheStartingDiskNumber);

	nuLong entriesCount = 0;

	CentralDirectoryFileHeader header;
	const auto saveExtraFieldsAndComments = m_Mode == ZipArchiveMode::Update;
	while (header.Read(m_Reader, saveExtraFieldsAndComments, m_Encoding))
	{
		auto entry = new ZipEntry(this, header);
		auto pEntry = natRefPointer<ZipEntry>{ entry };
		SafeRelease(entry);
		addEntry(std::move(pEntry));
		++entriesCount;
	}

	if (entriesCount != m_ZipEndOfCentralDirectory.NumberOfEntriesInTheCentralDirectory)
	{
		nat_Throw(InvalidData, "Number of entries is wrong."_nv);
	}
}

void natZipArchive::readEndOfCentralDirectory()
{
	m_Stream->SetPosition(NatSeek::End, -static_cast<nLong>(ZipEndOfCentralDirectory::SizeOfBlockWithoutSignature));
	if (!findSignatureBackward(m_Stream, ZipEndOfCentralDirectory::Signature))
	{
		nat_Throw(InvalidData, "Cannot find EOCD signature."_nv);
	}

	const auto eocdPosition = m_Stream->GetPosition();

	m_ZipEndOfCentralDirectory.Read(m_Reader, m_Encoding);

	if (m_ZipEndOfCentralDirectory.NumberOfThisDisk == Mask16Bit
		|| m_ZipEndOfCentralDirectory.OffsetOfStartOfCentralDirectoryWithRespectToTheStartingDiskNumber == Mask32Bit
		|| m_ZipEndOfCentralDirectory.NumberOfEntriesInTheCentralDirectory == Mask16Bit)
	{
		m_Stream->SetPosition(NatSeek::Beg, eocdPosition - Zip64EndOfCentralDirectoryLocator::SizeOfBlockWithoutSignature);

		if (findSignatureBackward(m_Stream, Zip64EndOfCentralDirectoryLocator::Signature))
		{
			m_Zip64EndOfCentralDirectoryLocator.Read(m_Reader);

			if (m_Zip64EndOfCentralDirectoryLocator.OffsetOfZip64EOCD > static_cast<nuLong>(std::numeric_limits<nLong>::max()))
			{
				nat_Throw(InvalidData, "OffsetOfZip64EOCD is too big."_nv);
			}

			m_Stream->SetPosition(NatSeek::Beg, static_cast<nLong>(m_Zip64EndOfCentralDirectoryLocator.OffsetOfZip64EOCD));

			m_Zip64EndOfCentralDirectory.Read(m_Reader);
		}
	}
}

void natZipArchive::removeEntry(ZipEntry* entry)
{
	m_EntriesMap.erase(entry->m_CentralDirectoryFileHeader.Filename);
}

nBool natZipArchive::findSignatureBackward(natStream* stream, nuInt signature)
{
	const auto pStream = stream;
	const auto signatureToFind = signature;
	nuInt currentSignature{};

	nByte findingBuffer[FindingBufferSize];
	size_t bufferIndex;

	auto signatureFound = false, streamReachBegin = false;

	while (!signatureFound && !streamReachBegin)
	{
		std::tie(streamReachBegin, bufferIndex) = readStreamBackward(pStream, findingBuffer, sizeof findingBuffer);
		assert(bufferIndex < sizeof findingBuffer);
		while (!signatureFound)
		{
			currentSignature = (currentSignature << 8) | static_cast<nuInt>(findingBuffer[bufferIndex]);
			if (currentSignature == signatureToFind)
			{
				signatureFound = true;
			}
			else
			{
				if (bufferIndex > 0)
				{
					--bufferIndex;
				}
				else
				{
					break;
				}
			}
		}
	}

	if (signatureFound)
	{
		pStream->SetPosition(NatSeek::Cur, bufferIndex);
		return true;
	}

	return false;
}

std::pair<nBool, size_t> natZipArchive::readStreamBackward(natStream* stream, nData buffer, size_t bufferSize)
{
	const auto pStream = stream;
	const auto bufSize = static_cast<nLong>(bufferSize);

	if (pStream->GetPosition() >= static_cast<nLen>(bufSize))
	{
		pStream->SetPosition(NatSeek::Cur, -bufSize);
		pStream->ReadBytes(buffer, bufSize);
		pStream->SetPosition(NatSeek::Cur, -bufSize);
		return { false, static_cast<size_t>(bufSize - 1) };
	}
	
	const auto readBytes = pStream->GetPosition();
	pStream->SetPosition(NatSeek::Beg, 0);
	pStream->ReadBytes(buffer, readBytes);
	pStream->SetPosition(NatSeek::Beg, 0);
	return { true, static_cast<size_t>(bufSize - 1) };
}
