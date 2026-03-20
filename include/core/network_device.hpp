#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace htop_killer::core {

struct NetworkDevice {
    std::string   ip;
    std::string   mac;
    std::string   hostname;
    std::string   vendor;
    std::string   iface;
    bool          is_gateway = false;
    bool          reachable  = true;

    std::uint64_t rx_bytes   = 0;
    std::uint64_t tx_bytes   = 0;
    double        rx_rate    = 0.0;
    double        tx_rate    = 0.0;

    std::uint32_t open_ports = 0;
    std::chrono::steady_clock::time_point first_seen{};
    std::chrono::steady_clock::time_point last_seen{};
};

struct NetworkDeviceList {
    std::vector<NetworkDevice>            devices;
    std::string                           local_ip;
    std::string                           local_mac;
    std::string                           gateway_ip;
    std::string                           subnet;
    std::chrono::steady_clock::time_point last_scan{};
    bool                                  scanning = false;
};

} // namespace htop_killer::core
