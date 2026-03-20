#pragma once

#include "core/types.hpp"

#include <atomic>
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
    void        scan_loop();
    void        refresh_arp();

    std::string best_iface();
    std::string read_local_ip(const std::string& iface);
    std::string read_local_mac(const std::string& iface);
    std::string read_gateway();

    mutable std::mutex mutex_;
    core::LanStats     state_;

    std::jthread     thread_;
    std::atomic_bool running_{false};
};

} // namespace htop_killer::collectors
