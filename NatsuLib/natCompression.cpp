#include "stdafx.h"
#include "natCompression.h"
#include "natEncoding.h"
#include "natCryptography.h"

using namespace NatsuLib;

// 在写入时被odr-use了，所以需要提供定义
constexpr nuInt natZipArchive::Mask32Bit;
constexpr nuShort natZipArchive::Mask16Bit;

natZipArchive::ZipEntry::~ZipEntry()
{
}

nStrView natZipArchive::ZipEntry::GetEntryName() const noexcept
{
	return m_CentralDirectoryFileHeader.Filename;
}

nuLong natZipArchive::ZipEntry::GetCompressedSize() const noexcept
{
	return m_CentralDirectoryFileHeader.CompressedSize;
}

nuLong natZipArchive::ZipEntry::GetUncompressedSize() const noexcept
{
	return m_CentralDirectoryFileHeader.UncompressedSize;
}

void natZipArchive::ZipEntry::Delete()
{
	if (!m_Archive)
	{
		return;
	}

	if (m_CurrentOpeningForWrite)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Cannot delete a entry while opening for write."_nv);
	}

	if (m_Archive->m_Mode != ZipArchiveMode::Update)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Cannot delete a entry while mode is not ZipArchiveMode::Update."_nv);
	}

	m_Archive->removeEntry(this);
	m_Archive = nullptr;
}

natRefPointer<natStream> natZipArchive::ZipEntry::Open()
{
	if (!m_Archive)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Archive or this entry has already disposed."_nv);
	}

	switch (m_Archive->m_Mode)
	{
	case ZipArchiveMode::Create:
		return openForCreate();
	case ZipArchiveMode::Read:
		return openForRead();
	case ZipArchiveMode::Update:
		return openForUpdate();
	default:
		assert(!"Invalid m_Mode.");
		nat_Throw(natErrException, NatErr_InternalErr, "Invalid m_Mode."_nv);
	}
}

void natZipArchive::ZipEntry::SetPassword()
{
	m_Password.reset();
}

void natZipArchive::ZipEntry::SetPassword(ncData password, size_t passwordLength)
{
	if (m_Password)
	{
		m_Password.value().assign(password, password + passwordLength);
	}
	else
	{
		m_Password.emplace(password, password + passwordLength);
	}
}

void natZipArchive::ZipEntry::SetPassword(nStrView passwordStr)
{
	SetPassword(reinterpret_cast<ncData>(passwordStr.data()), passwordStr.size() * sizeof(nStrView::CharType));
}

natZipArchive::ZipEntry::DecryptStatus natZipArchive::ZipEntry::GetDecryptStatus() const noexcept
{
	return m_DecryptStatus;
}

natZipArchive::ZipEntry::ZipEntry(natZipArchive* archive, CentralDirectoryFileHeader const& centralDirectoryFileHeader)
	: m_Archive{ archive }, m_OriginallyInArchive{ true }, m_CentralDirectoryFileHeader(centralDirectoryFileHeader), m_EverOpenedForWrite{ false }, m_CurrentOpeningForWrite{ false }, m_DecryptStatus{ DecryptStatus::NeedNotToDecrypt }
{
	if (m_CentralDirectoryFileHeader.GeneralPurposeBitFlag & static_cast<nuShort>(BitFlag::Encrypted))
	{
		m_DecryptStatus = DecryptStatus::NotDecryptYet;
	}
}

natRefPointer<natStream> natZipArchive::ZipEntry::openForRead()
{
	const auto offset = getOffsetOfCompressedData();
	auto compressedStream = make_ref<natSubStream>(m_Archive->m_Stream, offset, offset + m_CentralDirectoryFileHeader.CompressedSize);
	natRefPointer<natStream> uncompressor = compressedStream;

	if (m_CentralDirectoryFileHeader.GeneralPurposeBitFlag & static_cast<nuShort>(BitFlag::Encrypted))
	{
		if (!m_Password)
		{
			nat_Throw(EntryEncryptedException, "This entry is encrypted and no password provided."_nv);
		}

		const auto& password = m_Password.value();
		// 当前仅支持PKZipWeak加解密算法
		auto cryptoProcessor = make_ref<PKzipWeakAlgorithm>()->CreateDecryptor();
		auto pkZipWeakProcessor = static_cast<natRefPointer<PKzipWeakProcessor>>(cryptoProcessor);
		pkZipWeakProcessor->InitCipher(password.data(), password.size());
		pkZipWeakProcessor->InitHeaderFrom(compressedStream);
		m_DecryptStatus = pkZipWeakProcessor->CheckHeaderWithCrc32(m_CentralDirectoryFileHeader.Crc32) ? DecryptStatus::Success : DecryptStatus::Crc32CheckFailed;
		uncompressor = m_CryptoStream = make_ref<natCryptoStream>(compressedStream, cryptoProcessor, natCryptoStream::CryptoStreamMode::Read);
	}

	switch (static_cast<CompressionMethod>(m_CentralDirectoryFileHeader.CompressionMethod))
	{
	case CompressionMethod::Deflate:
		uncompressor = make_ref<natDeflateStream>(uncompressor);
		break;
	case CompressionMethod::Stored:
		break;
	case CompressionMethod::Deflate64:
	case CompressionMethod::BZip2:
	case CompressionMethod::LZMA:
	case CompressionMethod::Shrunk:
	case CompressionMethod::ReducedWithCompressionFactor1:
	case CompressionMethod::ReducedWithCompressionFactor2:
	case CompressionMethod::ReducedWithCompressionFactor3:
	case CompressionMethod::ReducedWithCompressionFactor4:
	case CompressionMethod::Imploded:
	case CompressionMethod::PKWareDCLImploded:
	case CompressionMethod::IBMTERSE:
	case CompressionMethod::IBMLZ77z:
	case CompressionMethod::PPMd:
	default:
		nat_Throw(NotImplementedException, "This compress method has not implemented yet (value is {0})."_nv, m_CentralDirectoryFileHeader.CompressionMethod);
	}

	return uncompressor;
}

natRefPointer<natStream> natZipArchive::ZipEntry::openForCreate()
{
	if (m_EverOpenedForWrite)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Already opened for write."_nv);
	}

	m_EverOpenedForWrite = true;
	m_CentralDirectoryFileHeader.CompressionMethod = static_cast<nuShort>(CompressionMethod::Deflate);

	return make_ref<ZipEntryWriteStream>(*this, createCompressor(m_Archive->m_Stream));
}

natRefPointer<natStream> natZipArchive::ZipEntry::openForUpdate()
{
	if (m_CurrentOpeningForWrite)
	{
		nat_Throw(natErrException, NatErr_IllegalState, "Only 1 stream should be opening at once in update mode."_nv);
	}

	m_EverOpenedForWrite = true;
	m_CurrentOpeningForWrite = true;
	m_CentralDirectoryFileHeader.CompressionMethod = static_cast<nuShort>(CompressionMethod::Deflate);

	return make_ref<DisposeCallbackStream>(getUncompressedData(), [this] (DisposeCallbackStream&)
	{
		m_CurrentOpeningForWrite = false;
	});
}

nLen natZipArchive::ZipEntry::getOffsetOfCompressedData()
{
	if (!m_OffsetOfCompressedData)
	{
		const auto localHeaderOffset = m_CentralDirectoryFileHeader.RelativeOffsetOfLocalHeader;
		m_Archive->m_Stream->SetPosition(NatSeek::Beg, localHeaderOffset);
		if (!LocalFileHeader::TrySkip(m_Archive->m_Reader))
		{
			nat_Throw(InvalidData);
		}
		m_OffsetOfCompressedData = m_Archive->m_Stream->GetPosition();
	}

	return m_OffsetOfCompressedData.value();
}

natRefPointer<natStream> const& natZipArchive::ZipEntry::getUncompressedData()
{
	if (!m_UncompressedData)
	{
		m_UncompressedData = make_ref<natMemoryStream>(0, true, true, true);

		if (m_OriginallyInArchive)
		{
			const auto decompressor = openForRead();
			m_UncompressedData->SetSize(decompressor->GetSize());
			decompressor->CopyTo(m_UncompressedData);
		}

		m_CentralDirectoryFileHeader.CompressionMethod = static_cast<nuShort>(CompressionMethod::Deflate);
	}

	return m_UncompressedData;
}

// 压缩流包装链：DisposeCallbackStream（若加密，用于获取crc32）
//	-> natCrc32Stream（计算crc32） -> natDeflateStream（压缩数据）
//		-> DisposeCallbackStream（若加密，用于写入到真正的输出流） -> natMemoryStream（若加密，用于保存未加密和压缩的数据）
//																\-> natCryptoStream（若加密，用于加密数据） -> stream（输出流）
// 压缩时先压缩再加密
natRefPointer<natStream> natZipArchive::ZipEntry::createCompressor(natRefPointer<natStream> stream)
{
	assert(stream && "stream should not be nullptr.");
	auto shouldEncrypt = false;
	auto compressor = std::move(stream);

	if (m_Password)
	{
		shouldEncrypt = true;
		const auto& password = m_Password.value();
		// 当前仅支持PKZipWeak加解密算法
		auto cryptoProcessor = make_ref<PKzipWeakAlgorithm>()->CreateEncryptor();
		auto pkZipWeakProcessor = static_cast<natRefPointer<PKzipWeakProcessor>>(cryptoProcessor);
		pkZipWeakProcessor->InitCipher(password.data(), password.size());
		compressor = m_CryptoStream = make_ref<natCryptoStream>(std::move(compressor), std::move(cryptoProcessor), natCryptoStream::CryptoStreamMode::Write);
		// 为了生成正确的头部，先缓存数据到内存流，在内容写入完成后再加密
		auto wrappedStream = make_ref<DisposeCallbackStream>(make_ref<natMemoryStream>(0, true, true, true),
			[this, originStream = std::move(compressor)] (DisposeCallbackStream& disposeCallbackStream)
			{
				writeSecurityMetadata(m_CryptoStream->GetUnderlyingStream());
				const auto underlyingStream = disposeCallbackStream.GetUnderlyingStream();
				underlyingStream->SetPosition(NatSeek::Beg, 0);
				underlyingStream->CopyTo(originStream);
			});
		compressor = std::move(wrappedStream);
		m_CentralDirectoryFileHeader.GeneralPurposeBitFlag |= static_cast<nuShort>(BitFlag::Encrypted);
	}

	compressor = make_ref<natCrc32Stream>(make_ref<natDeflateStream>(std::move(compressor), natDeflateStream::CompressionLevel::Optimal));

	if (shouldEncrypt)
	{
		// 流被析构之时获取crc32用于生成加密头部
		compressor = make_ref<DisposeCallbackStream>(std::move(compressor), [this] (DisposeCallbackStream& wrappedStream)
		{
			const auto crc32 = static_cast<natRefPointer<natCrc32Stream>>(wrappedStream.GetUnderlyingStream())->GetCrc32();
			static_cast<natRefPointer<PKzipWeakProcessor>>(m_CryptoStream->GetProcessor())->GenerateHeaderWithCrc32(crc32);
		});
	}

	return std::move(compressor);
}

void natZipArchive::ZipEntry::loadExtraFieldAndCompressedData()
{
	const auto stream = m_Archive->m_Stream;
	const auto reader = m_Archive->m_Reader;
	assert(stream && reader && "stream or reader should not be nullptr.");

	// 读取LocalFileHeader的附加字段
	if (m_OriginallyInArchive)
	{
		stream->SetPosition(NatSeek::Beg, m_CentralDirectoryFileHeader.RelativeOffsetOfLocalHeader + LocalFileHeader::OffsetToFilenameLength);

		const auto fileNameLength = reader->ReadPod<nuShort>();
		const auto extraFieldLength = reader->ReadPod<nuShort>();
		stream->SetPosition(NatSeek::Beg, fileNameLength);

		m_LocalHeaderFields.emplace();
		auto& fields = m_LocalHeaderFields.value();
		ExtraField field;

		const auto extraFieldStart = stream->GetPosition();
		while (field.ReadWithLimit(reader, extraFieldStart + extraFieldLength))
		{
			if (field.Tag != Zip64ExtraField::Tag)
			{
				fields.emplace_back(field);
			}
		}
	}

	// 读取原压缩数据
	if (m_OriginallyInArchive && !m_EverOpenedForWrite)
	{
		m_CachedCompressedData.emplace(static_cast<size_t>(m_CentralDirectoryFileHeader.CompressedSize));
		stream->SetPosition(NatSeek::Beg, getOffsetOfCompressedData());
		stream->ReadBytes(m_CachedCompressedData.value().data(), m_CentralDirectoryFileHeader.CompressedSize);
	}
}

void natZipArchive::ZipEntry::writeLocalFileHeaderAndData()
{
	const auto stream = m_Archive->m_Stream;
	const auto writer = m_Archive->m_Writer;

	if (m_UncompressedData)
	{
		m_CentralDirectoryFileHeader.UncompressedSize = m_UncompressedData->GetSize();

		const auto entryWriter = make_ref<ZipEntryWriteStream>(*this, createCompressor(stream));
		m_UncompressedData->SetPosition(NatSeek::Beg, 0);
		m_UncompressedData->CopyTo(entryWriter);
		m_UncompressedData.Reset();
	}
	else if (m_CachedCompressedData)
	{
		const auto& data = m_CachedCompressedData.value();
		if (!m_CentralDirectoryFileHeader.UncompressedSize)
		{
			m_CentralDirectoryFileHeader.CompressionMethod = static_cast<nuShort>(CompressionMethod::Stored);
		}
		LocalFileHeader::Write(writer, m_CentralDirectoryFileHeader, m_LocalHeaderFields, m_Archive->m_Encoding);
		stream->WriteBytes(data.data(), data.size());
	}
	else if (m_Archive->m_Mode == ZipArchiveMode::Update || !m_EverOpenedForWrite)
	{
		LocalFileHeader::Write(writer, m_CentralDirectoryFileHeader, m_LocalHeaderFields, m_Archive->m_Encoding);
		m_EverOpenedForWrite = true;
	}
}

void natZipArchive::ZipEntry::writeSecurityMetadata(natRefPointer<natStream> const& stream)
{
	if (m_CentralDirectoryFileHeader.GeneralPurposeBitFlag & static_cast<nuShort>(BitFlag::Encrypted))
	{
		assert(m_CryptoStream && "m_CryptoStream should not be nullptr.");
		// 当前仅支持PKZipWeak加解密算法
		const auto pkZipWeakProcessor = static_cast<natRefPointer<PKzipWeakProcessor>>(m_CryptoStream->GetProcessor());
		nByte tmpHeader[PKzipWeakProcessor::HeaderSize];
		if (!pkZipWeakProcessor->GetHeader(tmpHeader, sizeof tmpHeader))
		{
			nat_Throw(natErrException, NatErr_InternalErr, "Cannot get security header."_nv);
		}
		stream->WriteBytes(tmpHeader, sizeof tmpHeader);
	}
}

natZipArchive::ZipEntry::ZipEntryWriteStream::ZipEntryWriteStream(ZipEntry& entry, natRefPointer<natStream> stream, std::function<void(ZipEntryWriteStream&)> finishCallback)
	: natRefObjImpl{ std::move(stream) }, m_Entry{ entry }, m_InitialPosition{}, m_WroteData{}, m_UseZip64{}, m_FinishCallback{ move(finishCallback) }
{
	assert(m_InternalStream && "stream should not be nullptr.");
}

natZipArchive::ZipEntry::ZipEntryWriteStream::~ZipEntryWriteStream()
{
	finish();
}

nLen natZipArchive::ZipEntry::ZipEntryWriteStream::GetInitialPosition() const noexcept
{
	return m_InitialPosition;
}

nBool natZipArchive::ZipEntry::ZipEntryWriteStream::CanWrite() const
{
	return true;
}

nBool natZipArchive::ZipEntry::ZipEntryWriteStream::CanRead() const
{
	return false;
}

nBool natZipArchive::ZipEntry::ZipEntryWriteStream::CanResize() const
{
	return false;
}

nBool natZipArchive::ZipEntry::ZipEntryWriteStream::CanSeek() const
{
	return false;
}

nBool natZipArchive::ZipEntry::ZipEntryWriteStream::IsEndOfStream() const
{
	return m_InternalStream->IsEndOfStream();
}

nLen natZipArchive::ZipEntry::ZipEntryWriteStream::GetSize() const
{
	nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
}

void natZipArchive::ZipEntry::ZipEntryWriteStream::SetSize(nLen)
{
	nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
}

nLen natZipArchive::ZipEntry::ZipEntryWriteStream::GetPosition() const
{
	return m_InternalStream->GetPosition();
}

void natZipArchive::ZipEntry::ZipEntryWriteStream::SetPosition(NatSeek, nLong)
{
	nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
}

nLen natZipArchive::ZipEntry::ZipEntryWriteStream::ReadBytes(nData /*pData*/, nLen /*Length*/)
{
	nat_Throw(natErrException, NatErr_NotSupport, "The type of this stream does not support this operation."_nv);
}

nLen natZipArchive::ZipEntry::ZipEntryWriteStream::WriteBytes(ncData pData, nLen Length)
{
	if (!Length)
	{
		return 0;
	}

	if (!m_WroteData)
	{
		m_Entry.m_CentralDirectoryFileHeader.RelativeOffsetOfLocalHeader = m_Entry.m_Archive->m_Stream->GetPosition();
		m_UseZip64 = LocalFileHeader::Write(m_Entry.m_Archive->m_Writer, m_Entry.m_CentralDirectoryFileHeader, m_Entry.m_LocalHeaderFields, m_Entry.m_Archive->m_Encoding);
		const auto ultimateUnderlyingStream = GetUltimateUnderlyingStream();
		m_InitialPosition = ultimateUnderlyingStream->GetPosition();
		m_WroteData = true;
	}

	return m_InternalStream->WriteBytes(pData, Length);
}

void natZipArchive::ZipEntry::ZipEntryWriteStream::Flush()
{
	m_InternalStream->Flush();
}

void natZipArchive::ZipEntry::ZipEntryWriteStream::finish()
{
	GetUnderlyingStreamAs<natDeflateStream>()->Finish();
	const auto crc32Stream = GetUnderlyingStreamAs<natCrc32Stream>();
	assert(crc32Stream && "cannot get crc32stream.");
	m_Entry.m_CentralDirectoryFileHeader.Crc32 = crc32Stream->GetCrc32();
	m_Entry.m_CentralDirectoryFileHeader.UncompressedSize = crc32Stream->GetPosition();
	m_Entry.m_CentralDirectoryFileHeader.CompressedSize = crc32Stream->GetUltimateUnderlyingStream()->GetPosition() - m_InitialPosition;

	// 硬编码加入加密头的长度
	if (m_Entry.m_CentralDirectoryFileHeader.GeneralPurposeBitFlag & static_cast<nuShort>(BitFlag::Encrypted))
	{
		m_Entry.m_CentralDirectoryFileHeader.CompressedSize += PKzipWeakProcessor::HeaderSize;
	}

	if (m_WroteData)
	{
		// 已经写了 LocalFileHeader，补充Crc32以及大小信息（原本写入的是不完整的，需要修正）
		LocalFileHeader::WriteCrcAndSizes(m_Entry.m_Archive->m_Writer, m_Entry.m_CentralDirectoryFileHeader, m_UseZip64);
	}
	else
	{
		m_Entry.m_CentralDirectoryFileHeader.RelativeOffsetOfLocalHeader = m_Entry.m_Archive->m_Stream->GetPosition();
		LocalFileHeader::Write(m_Entry.m_Archive->m_Writer, m_Entry.m_CentralDirectoryFileHeader, m_Entry.m_LocalHeaderFields, m_Entry.m_Archive->m_Encoding);
	}

	if (m_FinishCallback)
	{
		m_FinishCallback(*this);
	}
}

natZipArchive::ZipEntry::ZipEntry(natZipArchive* archive, nStrView const& entryName)
	: m_Archive{ archive }, m_OriginallyInArchive{ false }, m_CentralDirectoryFileHeader{}, m_EverOpenedForWrite{ false }, m_CurrentOpeningForWrite{ false }, m_DecryptStatus{ DecryptStatus::NeedNotToDecrypt }
{
	m_CentralDirectoryFileHeader.Filename = entryName;
	m_CentralDirectoryFileHeader.FilenameLength = static_cast<nuShort>(m_CentralDirectoryFileHeader.Filename.size() * sizeof(nString::CharType));
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
		if (!m_Stream->CanRead() || !m_Stream->CanWrite() || !m_Stream->CanResize() || !m_Stream->CanSeek())
		{
			nat_Throw(natErrException, NatErr_InvalidArg, "stream should be readable, writable, resizable and seekable with ZipArchiveMode::Update mode."_nv);
		}
		m_Reader = make_ref<natBinaryReader>(m_Stream, Environment::Endianness::LittleEndian);
		m_Writer = make_ref<natBinaryWriter>(m_Stream, Environment::Endianness::LittleEndian);
		break;
	default:
		assert(!"Invalid mode value.");
	}
	
	internalOpen();
}

natZipArchive::~natZipArchive()
{
	close();
}

natZipArchive::ZipArchiveMode natZipArchive::GetOpenMode() const noexcept
{
	return m_Mode;
}

natRefPointer<natZipArchive::ZipEntry> natZipArchive::CreateEntry(nStrView entryName)
{
	auto entry = new ZipEntry(this, entryName);
	entry->SetDeleter();
	const natRefPointer<ZipEntry> pEntry{ entry };
	SafeRelease(entry);
	m_EntriesMap.emplace(pEntry->m_CentralDirectoryFileHeader.Filename, pEntry);
	return pEntry;
}

Linq<const natRefPointer<natZipArchive::ZipEntry>> natZipArchive::GetEntries() const
{
	return from(m_EntriesMap).select([](auto&& pair) -> const natRefPointer<ZipEntry>& { return pair.second; });
}

natRefPointer<natZipArchive::ZipEntry> natZipArchive::GetEntry(nStrView entryName) const
{
	const auto iter = m_EntriesMap.find(entryName);
	if (iter == m_EntriesMap.cend())
	{
		return {};
	}
	return iter->second;
}

void natZipArchive::addEntry(natRefPointer<ZipEntry> entry)
{
	m_EntriesMap.emplace(entry->m_CentralDirectoryFileHeader.Filename, std::move(entry));
}

void natZipArchive::close()
{
	switch (m_Mode)
	{
	case ZipArchiveMode::Create:
	case ZipArchiveMode::Update:
		writeToFile();
		break;
	case ZipArchiveMode::Read:
	default:
		assert(m_Mode == ZipArchiveMode::Read && "Invalid m_Mode.");
		break;
	}
}

void natZipArchive::writeToFile()
{
	if (m_Mode == ZipArchiveMode::Update)
	{
		for (auto&& entryPair : m_EntriesMap)
		{
			entryPair.second->loadExtraFieldAndCompressedData();
		}

		m_Stream->SetPosition(NatSeek::Beg, 0);
		m_Stream->SetSize(0);
	}

	for (auto&& entryPair : m_EntriesMap)
	{
		entryPair.second->writeLocalFileHeaderAndData();
	}

	const auto startOfCentralDirectory = m_Stream->GetPosition();
	for (auto&& entryPair : m_EntriesMap)
	{
		entryPair.second->m_CentralDirectoryFileHeader.Write(m_Writer, m_Encoding);
	}
	const auto endOfCentralDirectory = m_Stream->GetPosition();
	const auto sizeOfCentralDirectory = endOfCentralDirectory - startOfCentralDirectory;

	if (startOfCentralDirectory >= std::numeric_limits<nuInt>::max() || sizeOfCentralDirectory >= std::numeric_limits<nuInt>::max() || m_EntriesMap.size() > std::numeric_limits<nuShort>::max())
	{
		const auto zip64EOCDRecordStart = endOfCentralDirectory;
		Zip64EndOfCentralDirectory::Write(m_Writer, m_EntriesMap.size(), startOfCentralDirectory, sizeOfCentralDirectory);
		Zip64EndOfCentralDirectoryLocator::Write(m_Writer, zip64EOCDRecordStart);
	}

	ZipEndOfCentralDirectory::Write(m_Writer, m_EntriesMap.size(), startOfCentralDirectory, sizeOfCentralDirectory, m_ZipEndOfCentralDirectory.ArchiveComment, m_Encoding);
}

void natZipArchive::ExtraField::Read(natRefPointer<natBinaryReader> reader)
{
	Tag = reader->ReadPod<nuShort>();
	Size = reader->ReadPod<nuShort>();
	Data.resize(Size);
	reader->GetUnderlyingStream()->ReadBytes(Data.data(), Size);
}

nBool natZipArchive::ExtraField::ReadWithLimit(natRefPointer<natBinaryReader> reader, nLen endExtraField)
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

void natZipArchive::ExtraField::Write(natRefPointer<natBinaryWriter> writer) const
{
	writer->WritePod(Tag);
	writer->WritePod(Size);
	assert(Data.size() >= Size);
	writer->GetUnderlyingStream()->WriteBytes(Data.data(), Size);
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

	natBinaryReader reader{ make_ref<natExternMemoryStream>(extraField.Data.data(), extraField.Data.size(), true),
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

void natZipArchive::Zip64ExtraField::Write(natRefPointer<natBinaryWriter> writer)
{
	constexpr auto tag = Tag;

	writer->WritePod(tag);
	writer->WritePod(Size);
	if (UncompressedSize)
	{
		writer->WritePod(UncompressedSize.value());
	}
	if (CompressedSize)
	{
		writer->WritePod(CompressedSize.value());
	}
	if (LocalHeaderOffset)
	{
		writer->WritePod(LocalHeaderOffset.value());
	}
	if (StartDiskNumber)
	{
		writer->WritePod(StartDiskNumber.value());
	}
}

nBool natZipArchive::CentralDirectoryFileHeader::Read(natRefPointer<natBinaryReader> reader, nBool saveExtraFieldsAndComments, StringType encoding)
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
			Filename.Resize(FilenameLength);
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
				FileComment.Resize(FileCommentLength);
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

void natZipArchive::CentralDirectoryFileHeader::Write(natRefPointer<natBinaryWriter> writer, StringType encoding)
{
	constexpr auto signature = Signature;

	const auto stream = writer->GetUnderlyingStream();

	const auto filenameBytes = RuntimeEncoding<nString::UsingStringType>::Decode(Filename, encoding);
	if (filenameBytes.size() > std::numeric_limits<nuShort>::max())
	{
		nat_Throw(natErrException, NatErr_InternalErr, "Filename is too long."_nv);
	}
	FilenameLength = static_cast<nuShort>(filenameBytes.size());

	const auto fileCommentBytes = RuntimeEncoding<nString::UsingStringType>::Decode(FileComment, encoding);
	if (fileCommentBytes.size() > std::numeric_limits<nuShort>::max())
	{
		nat_Throw(natErrException, NatErr_InternalErr, "Filecomment is too long."_nv);
	}
	FileCommentLength = static_cast<nuShort>(fileCommentBytes.size());

	auto needZip64 = false;
	Zip64ExtraField zip64ExtraField;

	if (CompressedSize > std::numeric_limits<nuInt>::max())
	{
		needZip64 = true;
		zip64ExtraField.CompressedSize = CompressedSize;
	}

	if (UncompressedSize > std::numeric_limits<nuInt>::max())
	{
		needZip64 = true;
		zip64ExtraField.UncompressedSize = UncompressedSize;
	}

	if (DiskNumberStart > std::numeric_limits<nuShort>::max())
	{
		needZip64 = true;
		zip64ExtraField.StartDiskNumber = DiskNumberStart;
	}

	if (RelativeOffsetOfLocalHeader > std::numeric_limits<nuInt>::max())
	{
		needZip64 = true;
		zip64ExtraField.LocalHeaderOffset = RelativeOffsetOfLocalHeader;
	}

	writer->WritePod(signature);
	writer->WritePod(VersionMadeBySpecification);
	writer->WritePod(VersionMadeByCompatibility);
	writer->WritePod(VersionNeededToExtract);
	writer->WritePod(GeneralPurposeBitFlag);
	writer->WritePod(CompressionMethod);
	writer->WritePod(LastModified);
	writer->WritePod(Crc32);
	writer->WritePod(zip64ExtraField.CompressedSize ? Mask32Bit : static_cast<nuInt>(CompressedSize));
	writer->WritePod(zip64ExtraField.UncompressedSize ? Mask32Bit : static_cast<nuInt>(UncompressedSize));
	writer->WritePod(FilenameLength);
	writer->WritePod(ExtraFieldLength);
	writer->WritePod(FileCommentLength);
	writer->WritePod(zip64ExtraField.StartDiskNumber ? Mask16Bit : static_cast<nuShort>(DiskNumberStart));
	writer->WritePod(InternalFileAttributes);
	writer->WritePod(ExternalFileAttributes);
	writer->WritePod(zip64ExtraField.LocalHeaderOffset ? Mask32Bit : static_cast<nuInt>(RelativeOffsetOfLocalHeader));

	if (FilenameLength)
	{
		stream->WriteBytes(filenameBytes.data(), FilenameLength);
	}

	if (needZip64)
	{
		zip64ExtraField.Write(writer);
	}

	for (auto&& item : ExtraFields)
	{
		item.Write(writer);
	}

	if (FileCommentLength)
	{
		stream->WriteBytes(fileCommentBytes.data(), FileCommentLength);
	}
}

nBool natZipArchive::LocalFileHeader::TrySkip(natRefPointer<natBinaryReader> reader)
{
	constexpr nLong offsetToFilenameLength = 22;

	const auto stream = reader->GetUnderlyingStream();

	if (reader->ReadPod<nuInt>() != Signature)
	{
		return false;
	}

	if (stream->GetSize() < stream->GetPosition() + offsetToFilenameLength)
	{
		return false;
	}

	stream->SetPosition(NatSeek::Cur, offsetToFilenameLength);

	const auto filenameLength = reader->ReadPod<nuShort>();
	const auto extraFieldLength = reader->ReadPod<nuShort>();

	if (stream->GetSize() < stream->GetPosition() + filenameLength + extraFieldLength)
	{
		return false;
	}

	stream->SetPosition(NatSeek::Cur, filenameLength + extraFieldLength);

	return true;
}

nBool natZipArchive::LocalFileHeader::Write(natRefPointer<natBinaryWriter> writer, CentralDirectoryFileHeader& header, Optional<std::deque<ExtraField>> const& localFileHeaderFields, StringType encoding)
{
	assert(writer);

	decltype(auto) fileHeader = header;
	const auto stream = writer->GetUnderlyingStream();
	constexpr auto signature = Signature;

	const auto filenameBytes = RuntimeEncoding<nString::UsingStringType>::Decode(fileHeader.Filename, encoding);
	if (filenameBytes.size() > std::numeric_limits<nuShort>::max())
	{
		nat_Throw(natErrException, NatErr_InternalErr, "Filename is too long."_nv);
	}

	fileHeader.FilenameLength = static_cast<nuShort>(filenameBytes.size());

	auto needZip64 = false;
	Zip64ExtraField zip64ExtraField;

	if (fileHeader.CompressedSize > std::numeric_limits<nuInt>::max())
	{
		needZip64 = true;
		zip64ExtraField.CompressedSize = fileHeader.CompressedSize;
	}

	if (fileHeader.UncompressedSize > std::numeric_limits<nuInt>::max())
	{
		needZip64 = true;
		zip64ExtraField.UncompressedSize = fileHeader.UncompressedSize;
	}

	writer->WritePod(signature);
	writer->WritePod(fileHeader.VersionNeededToExtract);
	writer->WritePod(fileHeader.GeneralPurposeBitFlag);
	writer->WritePod(fileHeader.CompressionMethod);
	writer->WritePod(fileHeader.LastModified);
	writer->WritePod(fileHeader.Crc32);
	writer->WritePod(zip64ExtraField.CompressedSize ? Mask32Bit : static_cast<nuInt>(fileHeader.CompressedSize));
	writer->WritePod(zip64ExtraField.UncompressedSize ? Mask32Bit : static_cast<nuInt>(fileHeader.UncompressedSize));
	writer->WritePod(fileHeader.FilenameLength);
	writer->WritePod(fileHeader.ExtraFieldLength);
	stream->WriteBytes(filenameBytes.data(), filenameBytes.size());

	if (needZip64)
	{
		zip64ExtraField.Write(writer);
	}

	if (localFileHeaderFields)
	{
		auto&& fields = localFileHeaderFields.value();
		for (auto&& item : fields)
		{
			item.Write(writer);
		}
	}

	return needZip64;
}

void natZipArchive::LocalFileHeader::WriteCrcAndSizes(natRefPointer<natBinaryWriter> writer, CentralDirectoryFileHeader const& header, nBool usedZip64)
{
	const auto stream = writer->GetUnderlyingStream();
	const auto dataEndPosition = stream->GetPosition();

	stream->SetPosition(NatSeek::Beg, header.RelativeOffsetOfLocalHeader + OffsetToCrcFromHeaderStart);
	writer->WritePod(header.Crc32);
	if (usedZip64)
	{
		writer->WritePod(Mask32Bit);
		writer->WritePod(Mask32Bit);

		stream->SetPosition(NatSeek::Beg, header.RelativeOffsetOfLocalHeader + SizeOfLocalHeader + header.FilenameLength + Zip64ExtraField::OffsetToFirstField);
		writer->WritePod(header.UncompressedSize);
		writer->WritePod(header.CompressedSize);
	}
	else
	{
		writer->WritePod(static_cast<nuInt>(header.CompressedSize));
		writer->WritePod(static_cast<nuInt>(header.UncompressedSize));
	}

	stream->SetPosition(NatSeek::Beg, dataEndPosition);
}

void natZipArchive::ZipEndOfCentralDirectory::Read(natRefPointer<natBinaryReader> reader, StringType encoding)
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
			ArchiveComment.Resize(commentSize);
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

void natZipArchive::ZipEndOfCentralDirectory::Write(natRefPointer<natBinaryWriter> writer, nuLong numberOfEntries, nuLong startOfCentralDirectory, nuLong sizeOfCentralDirectory, nStrView archiveComment, StringType encoding)
{
	constexpr auto signature = Signature;

	const auto numberOfEntriesTruncated = numberOfEntries > std::numeric_limits<nuShort>::max() ? Mask16Bit : static_cast<nuShort>(numberOfEntries);
	const auto startOfCentralDirectoryTruncated = startOfCentralDirectory > std::numeric_limits<nuInt>::max() ? Mask32Bit : static_cast<nuInt>(startOfCentralDirectory);
	const auto sizeOfCentralDirectoryTruncated = sizeOfCentralDirectory > std::numeric_limits<nuInt>::max() ? Mask32Bit : static_cast<nuInt>(sizeOfCentralDirectory);

	writer->WritePod(signature);
	writer->WritePod(nuShort{});
	writer->WritePod(nuShort{});
	writer->WritePod(numberOfEntriesTruncated);
	writer->WritePod(numberOfEntriesTruncated);
	writer->WritePod(sizeOfCentralDirectoryTruncated);
	writer->WritePod(startOfCentralDirectoryTruncated);

	if (!archiveComment.empty())
	{
		assert(archiveComment.size() <= std::numeric_limits<nuShort>::max());

		const auto stream = writer->GetUnderlyingStream();

		if (encoding == nString::UsingStringType)
		{
			writer->WritePod(static_cast<nuShort>(archiveComment.size()));
			stream->WriteBytes(reinterpret_cast<ncData>(archiveComment.data()), archiveComment.size());
		}
		else
		{
			const auto buffer = RuntimeEncoding<nString::UsingStringType>::Decode(archiveComment, encoding);
			writer->WritePod(static_cast<nuShort>(buffer.size()));
			stream->WriteBytes(buffer.data(), buffer.size());
		}
	}
	else
	{
		writer->WritePod(nuShort{});
	}
}

void natZipArchive::Zip64EndOfCentralDirectoryLocator::Read(natRefPointer<natBinaryReader> reader)
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

void natZipArchive::Zip64EndOfCentralDirectoryLocator::Write(natRefPointer<natBinaryWriter> writer, nuLong zip64EOCDRecordStart)
{
	constexpr auto signature = Signature;

	writer->WritePod(signature);
	writer->WritePod(0u); // number of disk with start of zip64 eocd
	writer->WritePod(zip64EOCDRecordStart);
	writer->WritePod(1u); // total number of disks
}

void natZipArchive::Zip64EndOfCentralDirectory::Read(natRefPointer<natBinaryReader> reader)
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

void natZipArchive::Zip64EndOfCentralDirectory::Write(natRefPointer<natBinaryWriter> writer, nuLong numberOfEntries, nuLong startOfCentralDirectory, nuLong sizeOfCentralDirectory)
{
	constexpr auto signature = Signature;
	constexpr auto sizeWithoutExtraData = SizeWithoutExtraData;

	writer->WritePod(signature);
	writer->WritePod(sizeWithoutExtraData);
	writer->WritePod(static_cast<nuShort>(ZipVersionNeeded::Zip64));
	writer->WritePod(static_cast<nuShort>(ZipVersionNeeded::Zip64));
	writer->WritePod(0u);
	writer->WritePod(0u);
	writer->WritePod(numberOfEntries);
	writer->WritePod(numberOfEntries);
	writer->WritePod(sizeOfCentralDirectory);
	writer->WritePod(startOfCentralDirectory);
}

void natZipArchive::internalOpen()
{
	switch (m_Mode)
	{
	case ZipArchiveMode::Create:
		break;
	case ZipArchiveMode::Read:
		readEndOfCentralDirectory();
		readCentralDirectory();
		break;
	case ZipArchiveMode::Update:
	default:
		readEndOfCentralDirectory();
		readCentralDirectory();
		break;
	}
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
		entry->SetDeleter();
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

nBool natZipArchive::findSignatureBackward(natRefPointer<natStream> stream, nuInt signature)
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

std::pair<nBool, size_t> natZipArchive::readStreamBackward(natRefPointer<natStream> stream, nData buffer, size_t bufferSize)
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
