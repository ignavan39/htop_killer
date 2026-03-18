#include "collectors/linux_net_collector.hpp"

#include <fstream>
#include <sstream>
#include <string>

namespace htop_killer::collectors {

LinuxNetCollector::LinuxNetCollector() {
    prev_time_ = core::Clock::now();
}

core::NetStats LinuxNetCollector::collect() {
    std::ifstream file{"/proc/net/dev"};
    core::NetStats stats;
    stats.timestamp = core::Clock::now();

    const double dt = std::chrono::duration<double>(stats.timestamp - prev_time_).count();

    std::string line;
    std::getline(file, line);
    std::getline(file, line);

    std::unordered_map<std::string, Snapshot> curr;

    while (std::getline(file, line)) {
        std::istringstream ss{line};
        std::string iface;
        std::getline(ss, iface, ':');
        iface.erase(0, iface.find_first_not_of(" \t"));

        Snapshot snap;
        std::uint64_t dummy = 0;
        ss >> snap.rx_bytes >> snap.rx_packets
           >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy
           >> snap.tx_bytes >> snap.tx_packets;

        curr[iface] = snap;

        core::NetInterface ifc;
        ifc.name       = iface;
        ifc.rx_bytes   = snap.rx_bytes;
        ifc.tx_bytes   = snap.tx_bytes;
        ifc.rx_packets = snap.rx_packets;
        ifc.tx_packets = snap.tx_packets;

        if (prev_.contains(iface) && dt > 0.0) {
            const auto& p = prev_[iface];
            ifc.rx_rate   = static_cast<double>(snap.rx_bytes - p.rx_bytes) / dt;
            ifc.tx_rate   = static_cast<double>(snap.tx_bytes - p.tx_bytes) / dt;
        }

        stats.total_rx_rate += ifc.rx_rate;
        stats.total_tx_rate += ifc.tx_rate;

        if (iface != "lo")
            stats.interfaces.push_back(ifc);
    }

    prev_      = std::move(curr);
    prev_time_ = stats.timestamp;
    return stats;
}

} // namespace htop_killer::collectors
