#include "collectors/linux_mem_collector.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace htop_killer::collectors {

core::MemStats LinuxMemCollector::collect() {
    std::ifstream file{"/proc/meminfo"};
    if (!file.is_open())
        throw std::runtime_error{"Cannot open /proc/meminfo"};

    core::MemStats stats;
    stats.timestamp = core::Clock::now();

    std::string   key;
    std::uint64_t value = 0;
    std::string   unit;

    while (file >> key >> value) {
        std::getline(file, unit);

        if      (key == "MemTotal:")     stats.total_kb     = value;
        else if (key == "MemFree:")      stats.free_kb      = value;
        else if (key == "MemAvailable:") stats.available_kb = value;
        else if (key == "Cached:")       stats.cached_kb    = value;
        else if (key == "Buffers:")      stats.buffers_kb   = value;
        else if (key == "SwapTotal:")    stats.swap_total   = value;
        else if (key == "SwapFree:")     stats.swap_free    = value;
    }

    stats.used_kb   = stats.total_kb - stats.available_kb;
    stats.swap_used = stats.swap_total - stats.swap_free;
    stats.usage_pct = stats.total_kb > 0
                      ? static_cast<double>(stats.used_kb) / stats.total_kb * 100.0
                      : 0.0;
    stats.swap_pct  = stats.swap_total > 0
                      ? static_cast<double>(stats.swap_used) / stats.swap_total * 100.0
                      : 0.0;

    return stats;
}

} // namespace htop_killer::collectors
