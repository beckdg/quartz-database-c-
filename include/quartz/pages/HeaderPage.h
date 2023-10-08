#pragma once

#include "quartz/pages/BasePage.h"
#include "quartz/pages/PageLayouts.h"
#include "quartz/format/DatabaseHeader.h"
#include "quartz/format/FeatureFlags.h"
#include "quartz/format/Superblock.h"
#include "quartz/format/Versioning.h"
#include "quartz/storage/StorageConstants.h"

#include <cstdint>
#include <memory>

namespace quartz {
namespace pages {

class HeaderPage final : public BasePage {
public:
    HeaderPage();
    explicit HeaderPage(storage::Page page);

    std::unique_ptr<BasePage> clone() const override;

    void initFromFormat(const format::DatabaseHeader& dbHeader);

    const HeaderPageLayout* layout() const noexcept;
    HeaderPageLayout* layout() noexcept;

    const format::DatabaseHeader& databaseHeader() const noexcept;
    storage::PageId superblockPageId() const noexcept;
    void setSuperblockPageId(storage::PageId id) noexcept;
    std::uint64_t flags() const noexcept;
    void setFlags(std::uint64_t f) noexcept;
    std::uint32_t formatVersion() const noexcept;
    void setFormatVersion(std::uint32_t v) noexcept;

    Status validate() const override;
    void reset(storage::PageId newId) override;

    static HeaderPage create(std::uint32_t majorVersion,
                              std::uint32_t minorVersion,
                              std::uint64_t featureFlags);

private:
    HeaderPageLayout* mutableLayout() noexcept;
};

} // namespace pages
} // namespace quartz
