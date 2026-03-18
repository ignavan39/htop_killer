#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace htop_killer::core {

using Clock     = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

struct CpuCore {
    std::uint32_t id       = 0;
    double        usage    = 0.0;
    double        freq_mhz = 0.0;
};

struct CpuSnapshot {
    std::uint64_t user    = 0;
    std::uint64_t nice    = 0;
    std::uint64_t system  = 0;
    std::uint64_t idle    = 0;
    std::uint64_t iowait  = 0;
    std::uint64_t irq     = 0;
    std::uint64_t softirq = 0;
    std::uint64_t steal   = 0;
};

struct CpuStats {
    std::vector<CpuCore> cores;
    double               total_usage = 0.0;
    std::uint32_t        core_count  = 0;
    std::string          model_name;
    double               load_avg_1  = 0.0;
    double               load_avg_5  = 0.0;
    double               load_avg_15 = 0.0;
    TimePoint            timestamp;
};

struct MemStats {
    std::uint64_t total_kb     = 0;
    std::uint64_t free_kb      = 0;
    std::uint64_t available_kb = 0;
    std::uint64_t cached_kb    = 0;
    std::uint64_t buffers_kb   = 0;
    std::uint64_t used_kb      = 0;
    std::uint64_t swap_total   = 0;
    std::uint64_t swap_free    = 0;
    std::uint64_t swap_used    = 0;
    double        usage_pct    = 0.0;
    double        swap_pct     = 0.0;
    TimePoint     timestamp;
};

struct NetInterface {
    std::string   name;
    std::uint64_t rx_bytes   = 0;
    std::uint64_t tx_bytes   = 0;
    std::uint64_t rx_packets = 0;
    std::uint64_t tx_packets = 0;
    double        rx_rate    = 0.0;
    double        tx_rate    = 0.0;
};

struct NetStats {
    std::vector<NetInterface> interfaces;
    double                    total_rx_rate = 0.0;
    double                    total_tx_rate = 0.0;
    TimePoint                 timestamp;
};

struct DiskDevice {
    std::string   name;
    std::uint64_t read_bytes  = 0;
    std::uint64_t write_bytes = 0;
    double        read_rate   = 0.0;
    double        write_rate  = 0.0;
    double        util_pct    = 0.0;
};

struct DiskStats {
    std::vector<DiskDevice> devices;
    double                  total_read_rate  = 0.0;
    double                  total_write_rate = 0.0;
    TimePoint               timestamp;
};

struct ProcessInfo {
    std::uint32_t pid        = 0;
    std::uint32_t ppid       = 0;
    std::string   name;
    std::string   user;
    std::string   state;
    double        cpu_pct    = 0.0;
    std::uint64_t mem_rss_kb = 0;
    double        mem_pct    = 0.0;
    std::uint32_t threads    = 0;
    std::int32_t  nice       = 0;
    std::uint64_t uptime_s   = 0;
};

struct ProcessList {
    std::vector<ProcessInfo> processes;
    std::uint32_t            total_count    = 0;
    std::uint32_t            running_count  = 0;
    std::uint32_t            sleeping_count = 0;
    TimePoint                timestamp;
};

struct SystemSnapshot {
    CpuStats    cpu;
    MemStats    mem;
    NetStats    net;
    DiskStats   disk;
    ProcessList procs;
    TimePoint   timestamp;
};

} // namespace htop_killer::core
