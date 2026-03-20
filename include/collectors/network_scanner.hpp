#pragma once

#include "core/types.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace htop_killer::collectors {

struct NetworkDevice {
    std::string ip;
    std::string mac;
    std::string vendor;
    std::string hostname;
    std::string iface;
    bool        reachable   = false;
    std::uint64_t rx_bytes  = 0;
    std::uint64_t tx_bytes  = 0;
    std::uint32_t open_connections = 0;
    core::TimePoint first_seen;
    core::TimePoint last_seen;
};

struct LanSnapshot {
    std::vector<NetworkDevice> devices;
    std::string                local_ip;
    std::string                gateway_ip;
    std::string                subnet;
    core::TimePoint            timestamp;
};

class NetworkScanner {
public:
    NetworkScanner();

    [[nodiscard]] LanSnapshot snapshot() const;

    void scan();

private:
    struct ArpEntry {
        std::string ip;
        std::string mac;
        std::string iface;
        bool        complete = false;
    };

    [[nodiscard]] std::vector<ArpEntry>  read_arp_table();
    [[nodiscard]] std::string            resolve_hostname(const std::string& ip);
    [[nodiscard]] std::string            oui_vendor(const std::string& mac);
    [[nodiscard]] std::pair<std::string,std::string> read_gateway_and_local_ip();
    void                                 ping_sweep(const std::string& subnet_prefix, int timeout_ms = 300);
    void                                 count_connections(std::vector<NetworkDevice>& devs);

    mutable std::mutex          mutex_;
    LanSnapshot                 cached_;
};

} // namespace htop_killer::collectors
