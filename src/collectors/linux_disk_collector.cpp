#include "collectors/linux_disk_collector.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace htop_killer::collectors {

LinuxDiskCollector::LinuxDiskCollector() {
    prev_time_ = core::Clock::now();
}

core::DiskStats LinuxDiskCollector::collect() {
    std::ifstream file{"/proc/diskstats"};
    core::DiskStats stats;
    stats.timestamp = core::Clock::now();

    const double dt = std::chrono::duration<double>(stats.timestamp - prev_time_).count();

    std::unordered_map<std::string, Snapshot> curr;
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream ss{line};
        std::uint32_t major_n, minor_n;
        std::string devname;
        ss >> major_n >> minor_n >> devname;

        bool is_digit = false;
        for (char c : devname)
            if (std::isdigit(c)) { is_digit = true; break; }

        if (devname.find('p') != std::string::npos && is_digit) continue;
        if (devname.size() > 3 && devname[0] == 's' && devname[1] == 'd' &&
            std::isdigit(devname.back())) continue;

        std::uint64_t reads_compl, reads_merged, read_sectors, read_ms;
        std::uint64_t writes_compl, writes_merged, write_sectors, write_ms;
        std::uint64_t io_in_progress, io_ticks, weighted_io_ticks;

        ss >> reads_compl >> reads_merged >> read_sectors >> read_ms
           >> writes_compl >> writes_merged >> write_sectors >> write_ms
           >> io_in_progress >> io_ticks >> weighted_io_ticks;

        Snapshot snap{read_sectors, write_sectors, io_ticks};
        curr[devname] = snap;

        core::DiskDevice dev;
        dev.name        = devname;
        dev.read_bytes  = read_sectors  * kSectorBytes;
        dev.write_bytes = write_sectors * kSectorBytes;

        if (prev_.contains(devname) && dt > 0.0) {
            const auto& p   = prev_[devname];
            const double rb = static_cast<double>(read_sectors  - p.read_sectors)  * kSectorBytes;
            const double wb = static_cast<double>(write_sectors - p.write_sectors) * kSectorBytes;
            dev.read_rate   = rb / dt;
            dev.write_rate  = wb / dt;
            dev.util_pct    = std::min(
                static_cast<double>(io_ticks - p.io_ticks) / (dt * 1000.0) * 100.0, 100.0);
        }

        stats.total_read_rate  += dev.read_rate;
        stats.total_write_rate += dev.write_rate;
        stats.devices.push_back(std::move(dev));
    }

    prev_      = std::move(curr);
    prev_time_ = stats.timestamp;
    return stats;
}

} // namespace htop_killer::collectors
