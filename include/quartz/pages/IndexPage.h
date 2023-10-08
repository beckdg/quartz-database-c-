#pragma once

#include "quartz/pages/BasePage.h"
#include "quartz/pages/PageLayouts.h"
#include "quartz/storage/StorageConstants.h"

#include <cstdint>
#include <memory>

namespace quartz {
namespace pages {

class IndexPage final : public BasePage {
public:
    IndexPage();
    explicit IndexPage(storage::Page page);

    std::unique_ptr<BasePage> clone() const override;

    std::uint32_t nodeType() const noexcept;
    void setNodeType(std::uint32_t type) noexcept;
    std::uint32_t keyCount() const noexcept;
    void setKeyCount(std::uint32_t count) noexcept;
    std::uint32_t capacity() const noexcept;
    void setCapacity(std::uint32_t cap) noexcept;
    std::uint32_t flags() const noexcept;
    void setFlags(std::uint32_t f) noexcept;

    Status validate() const override;
    void reset(storage::PageId newId) override;

    static IndexPage create(storage::PageId pageId);

    const IndexPageLayout* layout() const noexcept;
    IndexPageLayout* layout() noexcept;

private:
    IndexPageLayout* mutableLayout() noexcept;
};

} // namespace pages
} // namespace quartz
