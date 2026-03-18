#pragma once

#include "core/types.hpp"

#include <unordered_map>

namespace htop_killer::collectors {

class LinuxDiskCollector {
public:
    LinuxDiskCollector();
    [[nodiscard]] core::DiskStats collect();

private:
    struct Snapshot {
        std::uint64_t read_sectors  = 0;
        std::uint64_t write_sectors = 0;
        std::uint64_t io_ticks      = 0;
    };

    static inline constexpr std::uint64_t kSectorBytes = 512;

    std::unordered_map<std::string, Snapshot> prev_;
    core::TimePoint                           prev_time_{};
};

} // namespace htop_killer::collectors
