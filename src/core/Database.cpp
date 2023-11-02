#include "quartz/core/Database.h"

#include "quartz/common/Config.h"
#include "quartz/diagnostics/ConsistencyChecker.h"
#include "quartz/pages/HeaderPage.h"
#include "quartz/recovery/RecoveryManager.h"

namespace quartz {
namespace core {

Database::Database()
    : tree_(space_, options_.btreeConfig)
    , walAdapter_(wal_) {}

Database::Database(DatabaseOptions options)
    : options_(std::move(options))
    , tree_(space_, options_.btreeConfig)
    , walAdapter_(wal_) {}

Database::~Database() {
    (void)close();
}

std::string Database::walPathFor(const std::string& dataPath) const {
    return dataPath + ".wal";
}

Status Database::open(const std::string& path, bool createIfMissing) {
    if (open_) {
        return Status::invalidArgument("Database: already open");
    }

    options_.dataPath = path;
    auto st = initializeStorage(path, createIfMissing);
    if (!st.ok()) return st;

    if (options_.enableWal) {
        st = initializeWal(walPathFor(path), createIfMissing);
        if (!st.ok()) {
            (void)file_.close();
            return st;
        }
    }

    if (options_.enableWal && options_.recoverOnOpen) {
        st = runRecovery();
        if (!st.ok()) {
            if (options_.enableWal) {
                (void)wal_.shutdown();
            }
            (void)file_.close();
            return st;
        }
    }

    if (options_.enableWal) {
        tree_.setWalSink(&walAdapter_);
    }

    open_ = true;
    ++stats_.openCount;
    return Status::success();
}

Status Database::close() {
    if (!open_) {
        return Status::success();
    }

    if (options_.enableWal) {
        tree_.setWalSink(nullptr);
        auto st = wal_.flush();
        if (!st.ok()) return st;
        st = wal_.shutdown();
        if (!st.ok()) return st;
    }

    auto st = file_.close();
    open_ = false;
    return st;
}

Status Database::initializeStorage(const std::string& path, bool create) {
    auto st = file_.open(path, create);
    if (!st.ok()) return st;

    if (create || file_.fileSize() < config::kPageSize) {
        auto headerPage = pages::HeaderPage::create(
            static_cast<std::uint32_t>(config::kVersionMajor),
            static_cast<std::uint32_t>(config::kVersionMinor), 0);
        st = file_.writePage(headerPage.page());
        if (!st.ok()) return st;
        st = file_.flush();
    }
    return Status::success();
}

Status Database::initializeWal(const std::string& walPath, bool create) {
    return wal_.initialize(walPath, create);
}

Status Database::runRecovery() {
    ++stats_.recoveryCount;
    return recovery::RecoveryManager::recover(wal_, tree_, lastRecovery_);
}

Status Database::insert(const btree::Key& key, format::PageReference value) {
    if (!open_) return Status::invalidArgument("Database: not open");
    auto st = tree_.insert(key, value);
    if (!st.ok()) return st;
    if (options_.enableWal) {
        st = wal_.flush();
    }
    return st;
}

Status Database::erase(const btree::Key& key) {
    if (!open_) return Status::invalidArgument("Database: not open");
    auto st = tree_.erase(key);
    if (!st.ok()) return st;
    if (options_.enableWal) {
        st = wal_.flush();
    }
    return st;
}

Status Database::find(const btree::Key& key, format::PageReference& out) const {
    if (!open_) return Status::invalidArgument("Database: not open");
    return tree_.find(key, out);
}

bool Database::contains(const btree::Key& key) const {
    if (!open_) return false;
    return tree_.contains(key);
}

Status Database::checkpoint(bool truncateWal) {
    if (!open_) return Status::invalidArgument("Database: not open");
    if (!options_.enableWal) {
        return Status::invalidArgument("Database: WAL not enabled");
    }
    const bool doTruncate = truncateWal || options_.truncateWalOnCheckpoint;
    wal::LogSequenceNumber lsn;
    auto st = recovery::RecoveryManager::checkpoint(tree_, wal_, doTruncate, lsn);
    if (st.ok()) {
        ++stats_.checkpointCount;
    }
    return st;
}

Status Database::validate() {
    if (!open_) return Status::invalidArgument("Database: not open");
    return diagnostics::ConsistencyChecker::check(*this);
}

DatabaseStatistics Database::statistics() const {
    auto snap = stats_;
    snap.btree = tree_.statistics();
    snap.space = space_.statistics().stats();
    if (options_.enableWal) {
        snap.wal = wal_.statistics();
    }
    return snap;
}

DatabaseContext Database::context() noexcept {
    return DatabaseContext{&file_, &space_, &tree_, options_.enableWal ? &wal_ : nullptr,
                           options_.enableWal ? &walAdapter_ : nullptr};
}

DatabaseContext Database::context() const noexcept {
    return DatabaseContext{const_cast<storage::DatabaseFile*>(&file_),
                           const_cast<space::SpaceManager*>(&space_),
                           const_cast<btree::BTree*>(&tree_),
                           options_.enableWal ? const_cast<wal::LogManager*>(&wal_) : nullptr,
                           options_.enableWal ? const_cast<wal::BTreeWalAdapter*>(&walAdapter_)
                                              : nullptr};
}

} // namespace core
} // namespace quartz
