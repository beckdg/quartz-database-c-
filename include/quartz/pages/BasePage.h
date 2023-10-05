#pragma once

#include "quartz/pages/PageTypes.h"
#include "quartz/storage/Page.h"
#include "quartz/storage/StorageConstants.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/common/Status.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace quartz {
namespace pages {

class BasePage {
public:
    virtual ~BasePage() = default;

    BasePage(BasePage&&) = default;
    BasePage& operator=(BasePage&&) = default;

    PageLayoutType layoutType() const noexcept {
        return toPageLayoutType(page_.type());
    }

    storage::PageId id() const noexcept { return page_.id(); }
    storage::PageType rawType() const noexcept { return page_.type(); }

    std::size_t size() const noexcept { return page_.size(); }
    std::size_t payloadSize() const noexcept { return page_.payloadSize(); }

    const storage::PageHeader& header() const noexcept { return page_.header(); }

    bool isValid() const noexcept { return page_.isValid(); }

    virtual Status validate() const = 0;

    virtual Status serialize(serialization::BinaryWriter& writer) const;
    virtual Status deserialize(serialization::BinaryReader& reader);

    virtual std::unique_ptr<BasePage> clone() const = 0;

    virtual void reset(storage::PageId newId);
    virtual void clear();

    const storage::Page& page() const noexcept { return page_; }
    storage::Page& page() noexcept { return page_; }

    std::string toString() const;

protected:
    BasePage() = default;
    explicit BasePage(storage::Page page);

private:
    storage::Page page_;
};

} // namespace pages
} // namespace quartz
