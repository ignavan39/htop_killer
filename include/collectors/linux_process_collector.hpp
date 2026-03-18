#pragma once

#include "core/types.hpp"

#include <unordered_map>

namespace htop_killer::collectors {

class LinuxProcessCollector {
public:
    LinuxProcessCollector();
    [[nodiscard]] core::ProcessList collect(std::uint64_t total_mem_kb,
                                            std::uint32_t cpu_cores);

private:
    struct ProcSnapshot {
        std::uint64_t utime  = 0;
        std::uint64_t stime  = 0;
        std::uint64_t cutime = 0;
        std::uint64_t cstime = 0;
    };

    core::TimePoint                                 prev_time_{};
    std::unordered_map<std::uint32_t, ProcSnapshot> prev_snaps_;
    long                                            hz_ = 100;
};

} // namespace htop_killer::collectors
