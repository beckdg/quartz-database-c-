#pragma once

#include "quartz/pages/BasePage.h"
#include "quartz/pages/PageLayouts.h"
#include "quartz/storage/StorageConstants.h"

#include <cstdint>
#include <memory>

namespace quartz {
namespace pages {

class MetadataPage final : public BasePage {
public:
    MetadataPage();
    explicit MetadataPage(storage::Page page);

    std::unique_ptr<BasePage> clone() const override;

    std::uint32_t entryCount() const noexcept;
    void setEntryCount(std::uint32_t count) noexcept;
    std::uint32_t version() const noexcept;
    void setVersion(std::uint32_t v) noexcept;

    Status validate() const override;
    void reset(storage::PageId newId) override;

    static MetadataPage create(storage::PageId pageId);

private:
    const MetadataPageLayout* layout() const noexcept;
    MetadataPageLayout* mutableLayout() noexcept;
};

} // namespace pages
} // namespace quartz
