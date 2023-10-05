#pragma once

#include "quartz/pages/BasePage.h"
#include "quartz/pages/PageLayouts.h"
#include "quartz/storage/StorageConstants.h"

#include <cstdint>
#include <memory>

namespace quartz {
namespace pages {

class DataPage final : public BasePage {
public:
    DataPage();
    explicit DataPage(storage::Page page);

    std::unique_ptr<BasePage> clone() const override;

    std::uint16_t freeSpaceOffset() const noexcept;
    void setFreeSpaceOffset(std::uint16_t offset) noexcept;
    std::uint16_t slotCount() const noexcept;
    void setSlotCount(std::uint16_t count) noexcept;

    std::size_t availableSpace() const noexcept;

    Status validate() const override;
    void reset(storage::PageId newId) override;

    static DataPage create(storage::PageId pageId);

private:
    const DataPageLayout* layout() const noexcept;
    DataPageLayout* mutableLayout() noexcept;
};

} // namespace pages
} // namespace quartz
