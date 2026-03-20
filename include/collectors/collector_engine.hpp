#pragma once

#include "core/data_store.hpp"
#include "collectors/linux_cpu_collector.hpp"
#include "collectors/linux_mem_collector.hpp"
#include "collectors/linux_net_collector.hpp"
#include "collectors/linux_disk_collector.hpp"
#include "collectors/linux_process_collector.hpp"
#include "collectors/linux_lan_collector.hpp"

#include <atomic>
#include <chrono>
#include <thread>

namespace htop_killer::collectors {

class CollectorEngine {
public:
    explicit CollectorEngine(core::DataStore& store,
                             std::chrono::milliseconds interval = std::chrono::seconds{1});
    ~CollectorEngine();

    CollectorEngine(const CollectorEngine&)            = delete;
    CollectorEngine& operator=(const CollectorEngine&) = delete;

    void start();
    void stop();

    [[nodiscard]] bool running() const noexcept { return running_.load(); }

private:
    void loop();

    core::DataStore&          store_;
    std::chrono::milliseconds interval_;
    std::atomic_bool          running_{false};
    std::jthread              thread_;

    LinuxCpuCollector     cpu_;
    LinuxMemCollector     mem_;
    LinuxNetCollector     net_;
    LinuxDiskCollector    disk_;
    LinuxProcessCollector procs_;
    LinuxLanCollector     lan_;
};

} // namespace htop_killer::collectors
