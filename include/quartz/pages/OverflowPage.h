#pragma once

#include "quartz/pages/BasePage.h"
#include "quartz/pages/PageLayouts.h"
#include "quartz/storage/StorageConstants.h"

#include <cstdint>
#include <memory>

namespace quartz {
namespace pages {

class OverflowPage final : public BasePage {
public:
    OverflowPage();
    explicit OverflowPage(storage::Page page);

    std::unique_ptr<BasePage> clone() const override;

    storage::PageId nextPageId() const noexcept;
    void setNextPageId(storage::PageId id) noexcept;
    bool hasNextPage() const noexcept;
    std::uint32_t payloadSize() const noexcept;
    void setPayloadSize(std::uint32_t size) noexcept;
    std::size_t remainingCapacity() const noexcept;

    Status validate() const override;
    void reset(storage::PageId newId) override;

    static OverflowPage create(storage::PageId pageId);

private:
    const OverflowPageLayout* layout() const noexcept;
    OverflowPageLayout* mutableLayout() noexcept;
};

} // namespace pages
} // namespace quartz
