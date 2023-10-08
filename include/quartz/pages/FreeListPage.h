#pragma once

#include "quartz/pages/BasePage.h"
#include "quartz/pages/PageLayouts.h"
#include "quartz/storage/StorageConstants.h"

#include <cstdint>
#include <memory>

namespace quartz {
namespace pages {

class FreeListPage final : public BasePage {
public:
    FreeListPage();
    explicit FreeListPage(storage::Page page);

    std::unique_ptr<BasePage> clone() const override;

    std::uint32_t freeCount() const noexcept;
    std::uint32_t capacity() const noexcept;
    bool isFull() const noexcept;
    bool isEmpty() const noexcept;

    storage::PageId freePage(std::size_t index) const noexcept;
    Status setFreePage(std::size_t index, storage::PageId pageId) noexcept;

    Status addFreePage(storage::PageId pageId) noexcept;
    void clearFreePages() noexcept;

    Status validate() const override;
    void reset(storage::PageId newId) override;

    static FreeListPage create(storage::PageId pageId);

private:
    const FreeListPageLayout* layout() const noexcept;
    FreeListPageLayout* mutableLayout() noexcept;
};

} // namespace pages
} // namespace quartz
