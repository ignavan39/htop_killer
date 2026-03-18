#pragma once

#include "core/history.hpp"
#include "core/types.hpp"

#include <mutex>
#include <shared_mutex>

namespace htop_killer::core {

class DataStore {
public:
    void update(SystemSnapshot snap) {
        std::unique_lock lock{mutex_};
        current_ = snap;
        history_.update(snap);
    }

    [[nodiscard]] SystemSnapshot snapshot() const {
        std::shared_lock lock{mutex_};
        return current_;
    }

    [[nodiscard]] History history() const {
        std::shared_lock lock{mutex_};
        return history_;
    }

private:
    mutable std::shared_mutex mutex_;
    SystemSnapshot            current_{};
    History                   history_{};
};

} // namespace htop_killer::core
