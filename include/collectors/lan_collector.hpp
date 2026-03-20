#pragma once

#include "core/types.hpp"

#include <chrono>
#include <string>
#include <unordered_map>

namespace htop_killer::collectors {

class LanCollector {
public:
    LanCollector();
    [[nodiscard]] core::LanStats collect();

private:
    struct DeviceState {
        std::uint64_t rx_bytes  = 0;
        std::uint64_t tx_bytes  = 0;
        core::TimePoint last_seen{};
        core::TimePoint first_seen{};
        double latency_ms = -1.0;
        std::string hostname;
        std::string vendor;
    };

    struct ArpEntry {
        std::string ip;
        std::string mac;
        std::string iface;
        int         flags = 0;
    };

    [[nodiscard]] std::vector<ArpEntry> read_arp_table();
    [[nodiscard]] std::string           read_gateway();
    [[nodiscard]] std::string           read_local_ip(const std::string& iface);
    [[nodiscard]] double                ping_latency(const std::string& ip);
    [[nodiscard]] std::string           resolve_hostname(const std::string& ip);
    [[nodiscard]] std::string           oui_vendor(const std::string& mac);
    [[nodiscard]] std::uint32_t         count_open_ports(const std::string& ip);

    std::unordered_map<std::string, DeviceState> known_;  // keyed by MAC
    core::TimePoint                              prev_time_{};
    int                                          ping_round_ = 0; // ping one device per tick
};

} // namespace htop_killer::collectors
