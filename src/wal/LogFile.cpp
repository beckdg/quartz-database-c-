#include "quartz/wal/LogFile.h"

#include "quartz/common/Config.h"
#include "quartz/format/MagicNumbers.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BufferView.h"

#include <cstring>

namespace quartz {
namespace wal {

LogFile::LogFile()
    : file_(&ownedFile_) {}

LogFile::LogFile(storage::DatabaseFile& file)
    : file_(&file) {
    appendOffset_ = file.fileSize() > WalFileHeader::kSize ? file.fileSize() : WalFileHeader::kSize;
}

Status LogFile::create(const std::string& path) {
    ownsFile_ = true;
    file_ = &ownedFile_;
    auto st = ownedFile_.open(path, true);
    if (!st.ok()) return st;
    appendOffset_ = WalFileHeader::kSize;
    return writeHeader();
}

Status LogFile::open(const std::string& path, bool createIfMissing) {
    ownsFile_ = true;
    file_ = &ownedFile_;
    auto st = ownedFile_.open(path, createIfMissing);
    if (!st.ok()) return st;

    WalFileHeader header{};
    st = readHeader(header);
    if (!st.ok()) {
        if (!createIfMissing) return st;
        appendOffset_ = WalFileHeader::kSize;
        return writeHeader();
    }
    appendOffset_ = file_->fileSize();
    return Status::success();
}

Status LogFile::close() {
    if (file_ == nullptr) {
        return Status::success();
    }
    auto st = file_->close();
    file_ = nullptr;
    appendOffset_ = WalFileHeader::kSize;
    return st;
}

bool LogFile::isOpen() const noexcept {
    return file_ != nullptr && file_->isOpen();
}

const std::string& LogFile::path() const noexcept {
    static const std::string kEmpty;
    return file_ != nullptr ? file_->path() : kEmpty;
}

Status LogFile::append(serialization::BufferView data) {
    if (file_ == nullptr || !file_->isOpen()) {
        return Status::ioError("LogFile: file not open");
    }
    auto st = file_->seek(appendOffset_);
    if (!st.ok()) return st;
    st = file_->writeBytes(data.data(), data.size());
    if (!st.ok()) return st;
    appendOffset_ += data.size();
    return alignAppendOffset();
}

Status LogFile::appendRecord(const LogRecord& record) {
    serialization::Buffer buf;
    serialization::BinaryWriter writer(buf);
    std::uint32_t recordSize = 0;
    auto st = writer.write(recordSize);
    if (!st.ok()) return st;
    const auto headerStart = writer.tell();
    st = record.serialize(writer);
    if (!st.ok()) return st;
    recordSize = static_cast<std::uint32_t>(writer.tell() - headerStart);
    st = writer.overwriteAt(0, &recordSize, sizeof(recordSize));
    if (!st.ok()) return st;
    return append(serialization::BufferView(buf));
}

Status LogFile::readBytes(std::uint64_t offset, void* buffer, std::size_t count) const {
    if (file_ == nullptr || !file_->isOpen()) {
        return Status::ioError("LogFile: file not open");
    }
    auto st = file_->seek(offset);
    if (!st.ok()) return st;
    return file_->readBytes(buffer, count);
}

Status LogFile::readRecord(std::uint64_t offset, LogRecord& record, std::uint64_t& bytesRead) const {
    if (file_ == nullptr || !file_->isOpen()) {
        return Status::ioError("LogFile: file not open");
    }
    std::uint32_t recordSize = 0;
    auto st = readBytes(offset, &recordSize, sizeof(recordSize));
    if (!st.ok()) return st;
    if (recordSize == 0) {
        return Status::corruption("LogFile: zero-length record");
    }

    serialization::Buffer buf;
    buf.resize(sizeof(recordSize) + recordSize);
    std::memcpy(buf.data(), &recordSize, sizeof(recordSize));
    st = readBytes(offset + sizeof(recordSize), buf.data() + sizeof(recordSize), recordSize);
    if (!st.ok()) return st;

    serialization::BinaryReader reader{serialization::BufferView(buf)};
    st = reader.skip(sizeof(recordSize));
    if (!st.ok()) return st;
    st = record.deserialize(reader);
    if (!st.ok()) return st;

    bytesRead = sizeof(recordSize) + recordSize;
    const auto remainder = bytesRead % kLogRecordAlignment;
    if (remainder != 0) {
        bytesRead += kLogRecordAlignment - remainder;
    }
    return Status::success();
}

Status LogFile::flush() {
    if (file_ == nullptr) {
        return Status::ioError("LogFile: file not open");
    }
    return file_->flush();
}

Status LogFile::truncate(std::uint64_t newSize) {
    if (file_ == nullptr) {
        return Status::ioError("LogFile: file not open");
    }
    if (newSize < WalFileHeader::kSize) {
        return Status::invalidArgument("LogFile: truncate below header size");
    }
    auto st = file_->resize(newSize);
    if (!st.ok()) return st;
    appendOffset_ = newSize;
    return Status::success();
}

std::uint64_t LogFile::fileSize() const noexcept {
    return file_ != nullptr ? file_->fileSize() : 0;
}

std::uint64_t LogFile::dataOffset() const noexcept {
    return WalFileHeader::kSize;
}

std::uint64_t LogFile::appendOffset() const noexcept {
    return appendOffset_;
}

Status LogFile::validate() const {
    if (file_ == nullptr || !file_->isOpen()) {
        return Status::ioError("LogFile: file not open");
    }
    WalFileHeader header{};
    auto st = readHeader(header);
    if (!st.ok()) return st;
    if (header.magic != format::MagicNumbers::kJournalMagic) {
        return Status::corruption("LogFile: invalid WAL magic");
    }
    if (header.version != kWalFormatVersion) {
        return Status::corruption("LogFile: unsupported WAL version");
    }
    return Status::success();
}

Status LogFile::writeHeader() {
    WalFileHeader header{};
    header.magic = format::MagicNumbers::kJournalMagic;
    header.version = kWalFormatVersion;
    header.pageSize = static_cast<std::uint32_t>(config::kPageSize);
    auto st = file_->seek(0);
    if (!st.ok()) return st;
    return file_->writeBytes(&header, sizeof(header));
}

Status LogFile::readHeader(WalFileHeader& header) const {
    if (file_ == nullptr || !file_->isOpen()) {
        return Status::ioError("LogFile: file not open");
    }
    if (file_->fileSize() < WalFileHeader::kSize) {
        return Status::corruption("LogFile: file too small for header");
    }
    auto st = file_->seek(0);
    if (!st.ok()) return st;
    return file_->readBytes(&header, sizeof(header));
}

Status LogFile::alignAppendOffset() {
    const auto remainder = appendOffset_ % kLogRecordAlignment;
    if (remainder == 0) {
        return Status::success();
    }
    const auto padding = kLogRecordAlignment - remainder;
    std::uint8_t pad[8] = {};
    auto st = file_->writeBytes(pad, padding);
    if (!st.ok()) return st;
    appendOffset_ += padding;
    return Status::success();
}

} // namespace wal
} // namespace quartz
