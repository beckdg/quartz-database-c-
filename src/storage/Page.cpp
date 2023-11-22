#include "quartz/storage/Page.h"

#include <cstring>
#include <sstream>

namespace quartz {
namespace storage {

Page::Page() noexcept
    : header_(PageHeader::make(kInvalidPageId, PageType::Invalid))
    , data_(new std::uint8_t[kPageSize]())
    , dirty_(false)
    , pinCount_(0)
    , ownsBuffer_(true) {

    std::memcpy(data_, &header_, kPageHeaderSize);
}

Page::Page(PageId id, PageType type)
    : header_(PageHeader::make(id, type))
    , data_(new std::uint8_t[kPageSize]())
    , dirty_(false)
    , pinCount_(0)
    , ownsBuffer_(true) {

    std::memcpy(data_, &header_, kPageHeaderSize);
}

Page::~Page() noexcept {
    if (ownsBuffer_) {
        delete[] data_;
    }
}

Page::Page(Page&& other) noexcept
    : header_(other.header_)
    , data_(other.data_)
    , dirty_(other.dirty_)
    , pinCount_(other.pinCount_)
    , ownsBuffer_(other.ownsBuffer_) {

    other.data_ = nullptr;
    other.ownsBuffer_ = false;
    other.header_ = PageHeader::make(kInvalidPageId, PageType::Invalid);
    other.dirty_ = false;
    other.pinCount_ = 0;
}

Page& Page::operator=(Page&& other) noexcept {
    if (this != &other) {
        if (ownsBuffer_) {
            delete[] data_;
        }

        header_ = other.header_;
        data_ = other.data_;
        dirty_ = other.dirty_;
        pinCount_ = other.pinCount_;
        ownsBuffer_ = other.ownsBuffer_;

        other.data_ = nullptr;
        other.ownsBuffer_ = false;
        other.header_ = PageHeader::make(kInvalidPageId, PageType::Invalid);
        other.dirty_ = false;
        other.pinCount_ = 0;
    }
    return *this;
}

void Page::reset(PageId id, PageType type) noexcept {
    header_ = PageHeader::make(id, type);
    if (ownsBuffer_) {
        std::memset(data_, 0, kPageSize);
    }
    std::memcpy(data_, &header_, kPageHeaderSize);
    dirty_ = false;
    pinCount_ = 0;
}

void Page::zeroFill() noexcept {
    if (data_) {
        std::memset(data_, 0, kPageSize);
    }
}

std::string_view Page::toString() const {
    return std::string_view(reinterpret_cast<const char*>(data_), kPageSize);
}

} // namespace storage
} // namespace quartz
