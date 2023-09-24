#pragma once

#include "quartz/btree/BTree.h"
#include "quartz/btree/Key.h"
#include "quartz/common/NonCopyable.h"
#include "quartz/common/Status.h"
#include "quartz/core/DatabaseContext.h"
#include "quartz/core/DatabaseOptions.h"
#include "quartz/core/DatabaseStatistics.h"
#include "quartz/format/PageReference.h"
#include "quartz/recovery/RecoveryTypes.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/storage/DatabaseFile.h"
#include "quartz/wal/BTreeWalAdapter.h"
#include "quartz/wal/LogManager.h"

#include <string>

namespace quartz {
namespace core {

/// Top-level embedded storage engine coordinating file I/O, space, index, WAL, and recovery.
class Database : private NonCopyable {
public:
    Database();
    explicit Database(DatabaseOptions options);
    ~Database();

    Status open(const std::string& path, bool createIfMissing = true);
    Status close();

    bool isOpen() const noexcept { return open_; }

    Status insert(const btree::Key& key, format::PageReference value);
    Status erase(const btree::Key& key);
    Status find(const btree::Key& key, format::PageReference& out) const;
    bool contains(const btree::Key& key) const;

    Status checkpoint(bool truncateWal = false);
    Status validate();

    DatabaseStatistics statistics() const;
    DatabaseContext context() noexcept;
    DatabaseContext context() const noexcept;

    btree::BTree& tree() noexcept { return tree_; }
    const btree::BTree& tree() const noexcept { return tree_; }
    space::SpaceManager& space() noexcept { return space_; }
    const space::SpaceManager& space() const noexcept { return space_; }
    wal::LogManager& wal() noexcept { return wal_; }
    const wal::LogManager& wal() const noexcept { return wal_; }
    storage::DatabaseFile& file() noexcept { return file_; }
    const storage::DatabaseFile& file() const noexcept { return file_; }

    const DatabaseOptions& options() const noexcept { return options_; }
    const recovery::RecoveryResult& lastRecovery() const noexcept { return lastRecovery_; }

private:
    Status initializeStorage(const std::string& path, bool create);
    Status initializeWal(const std::string& walPath, bool create);
    Status runRecovery();
    std::string walPathFor(const std::string& dataPath) const;

    DatabaseOptions options_;
    bool open_ = false;

    storage::DatabaseFile file_;
    space::SpaceManager space_;
    btree::BTree tree_;
    wal::LogManager wal_;
    wal::BTreeWalAdapter walAdapter_;

    recovery::RecoveryResult lastRecovery_;
    mutable DatabaseStatistics stats_;
};

} // namespace core
} // namespace quartz
