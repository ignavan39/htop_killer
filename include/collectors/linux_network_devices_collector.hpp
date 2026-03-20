#pragma once

#include "core/network_device.hpp"

#include <chrono>
#include <string>
#include <unordered_map>

namespace htop_killer::collectors {

class LinuxNetworkDevicesCollector {
public:
    LinuxNetworkDevicesCollector();

    [[nodiscard]] core::NetworkDeviceList collect();
    void scan();

private:
    void        parse_arp_table(core::NetworkDeviceList& out);
    void        read_local_info(core::NetworkDeviceList& out);
    void        parse_conntrack(core::NetworkDeviceList& out);
    std::string resolve_hostname(const std::string& ip);
    std::string lookup_vendor(const std::string& mac);
    std::string read_gateway();

    core::NetworkDeviceList                            cache_;
    std::unordered_map<std::string, core::NetworkDevice> known_;
    std::chrono::steady_clock::time_point              last_scan_{};

    static constexpr std::chrono::seconds kScanInterval{15};
};

} // namespace htop_killer::collectors
