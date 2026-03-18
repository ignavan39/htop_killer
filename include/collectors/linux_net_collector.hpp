#pragma once

#include "core/types.hpp"

#include <chrono>
#include <unordered_map>

namespace htop_killer::collectors {

class LinuxNetCollector {
public:
    LinuxNetCollector();
    [[nodiscard]] core::NetStats collect();

private:
    struct Snapshot {
        std::uint64_t rx_bytes   = 0;
        std::uint64_t tx_bytes   = 0;
        std::uint64_t rx_packets = 0;
        std::uint64_t tx_packets = 0;
    };

    std::unordered_map<std::string, Snapshot> prev_;
    core::TimePoint                           prev_time_{};
};

} // namespace htop_killer::collectors
