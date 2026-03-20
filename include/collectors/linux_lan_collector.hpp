#pragma once

#include "core/types.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace htop_killer::collectors {

class LinuxLanCollector {
public:
    LinuxLanCollector();
    ~LinuxLanCollector();
    [[nodiscard]] core::LanStats collect();

private:
    struct HostSnapshot {
        std::uint64_t rx_bytes = 0;
        std::uint64_t tx_bytes = 0;
    };

    void sniffer_loop(const std::string& iface);

    struct HostCounters {
        std::atomic<std::uint64_t> rx{0};
        std::atomic<std::uint64_t> tx{0};
    };

    [[nodiscard]] std::unordered_map<std::string, std::string>   read_arp_table();
    [[nodiscard]] std::string                                    resolve_hostname(const std::string& ip);
    [[nodiscard]] std::string                                    oui_vendor(const std::string& mac);
    [[nodiscard]] std::string                                    read_gateway();
    [[nodiscard]] std::string                                    read_local_ip();
    [[nodiscard]] std::string                                    detect_iface();
    [[nodiscard]] std::unordered_map<std::string, std::uint32_t> read_tcp_connections();


    std::thread      sniffer_thread_;
    std::atomic_bool sniffer_running_{false};
    int              raw_fd_{-1};

    mutable std::mutex                              counters_mutex_;
    std::unordered_map<std::string, HostCounters*> counters_;  // ip → counters

    std::unordered_map<std::string, HostSnapshot> prev_bytes_;
    std::unordered_map<std::string, std::string>  hostname_cache_;
    core::TimePoint                               prev_time_{};
    std::string                                   gateway_;
    std::string                                   local_ip_;
    std::string                                   iface_;
};

} // namespace htop_killer::collectors
