#pragma once

#include "quartz/common/NonCopyable.h"
#include "quartz/storage/PageHeader.h"
#include "quartz/storage/StorageConstants.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace quartz {
namespace storage {

class Page : private NonCopyable {
public:
    Page() noexcept;
    explicit Page(PageId id, PageType type = PageType::Data);
    ~Page() noexcept;

    Page(Page&& other) noexcept;
    Page& operator=(Page&& other) noexcept;

    void reset(PageId id, PageType type = PageType::Data) noexcept;

    PageId id() const noexcept { return header_.pageId; }
    PageType type() const noexcept { return header_.pageType; }

    std::uint8_t* data() noexcept { return data_; }
    const std::uint8_t* data() const noexcept { return data_; }

    std::size_t size() const noexcept { return kPageSize; }
    std::size_t payloadSize() const noexcept { return header_.payloadSize; }
    void setPayloadSize(std::uint16_t size) noexcept { header_.payloadSize = size; }

    std::uint64_t generation() const noexcept { return header_.generation; }
    void setGeneration(std::uint64_t gen) noexcept { header_.generation = gen; }

    bool dirty() const noexcept { return dirty_; }
    void setDirty(bool d) noexcept { dirty_ = d; }

    int pinCount() const noexcept { return pinCount_; }
    void pin() noexcept { ++pinCount_; }
    void unpin() noexcept { if (pinCount_ > 0) --pinCount_; }

    std::uint8_t* payload() noexcept { return data_ + kPageHeaderSize; }
    const std::uint8_t* payload() const noexcept { return data_ + kPageHeaderSize; }

    PageHeader& header() noexcept { return header_; }
    const PageHeader& header() const noexcept { return header_; }

    bool isValid() const noexcept { return header_.isValid(); }

    void zeroFill() noexcept;

    std::string_view toString() const;

private:
    PageHeader header_;
    std::uint8_t* data_;
    bool dirty_ = false;
    int pinCount_ = 0;
    bool ownsBuffer_ = true;
};

} // namespace storage
} // namespace quartz
