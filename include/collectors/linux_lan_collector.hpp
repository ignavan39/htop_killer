#pragma once

#include "core/types.hpp"

#include <chrono>
#include <string>
#include <unordered_map>

namespace htop_killer::collectors {

class LinuxLanCollector {
public:
    LinuxLanCollector();
    [[nodiscard]] core::LanStats collect();

private:
    struct HostSnapshot {
        std::uint64_t rx_bytes = 0;
        std::uint64_t tx_bytes = 0;
    };

    [[nodiscard]] std::unordered_map<std::string, std::string>   read_arp_table();
    [[nodiscard]] std::string                                    resolve_hostname(const std::string& ip);
    [[nodiscard]] std::string                                    oui_vendor(const std::string& mac);
    [[nodiscard]] std::string                                    read_gateway();
    [[nodiscard]] std::string                                    read_local_ip();
    [[nodiscard]] std::unordered_map<std::string, HostSnapshot>  read_conntrack();
    [[nodiscard]] std::unordered_map<std::string, std::uint32_t> read_tcp_connections();

    std::unordered_map<std::string, HostSnapshot> prev_bytes_;
    std::unordered_map<std::string, std::string>  hostname_cache_;
    core::TimePoint                               prev_time_{};
    std::string                                   gateway_;
    std::string                                   local_ip_;
};

} // namespace htop_killer::collectors
