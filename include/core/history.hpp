#pragma once

#include "core/ring_buffer.hpp"
#include "core/types.hpp"

#include <cstddef>

namespace htop_killer::core {

inline constexpr std::size_t kHistoryLen = 120;

struct History {
    static inline constexpr std::size_t kMaxCores = 32;

    RingBuffer<double, kHistoryLen> cpu_total;
    RingBuffer<double, kHistoryLen> cpu_cores[kMaxCores];
    RingBuffer<double, kHistoryLen> mem_used_pct;
    RingBuffer<double, kHistoryLen> swap_used_pct;
    RingBuffer<double, kHistoryLen> net_rx_mbps;
    RingBuffer<double, kHistoryLen> net_tx_mbps;
    RingBuffer<double, kHistoryLen> disk_read_mbps;
    RingBuffer<double, kHistoryLen> disk_write_mbps;

    void update(const SystemSnapshot& snap) {
        cpu_total.push(snap.cpu.total_usage);

        const auto n = std::min(snap.cpu.cores.size(), kMaxCores);
        for (std::size_t i = 0; i < n; ++i)
            cpu_cores[i].push(snap.cpu.cores[i].usage);

        mem_used_pct.push(snap.mem.usage_pct);
        swap_used_pct.push(snap.mem.swap_pct);
        net_rx_mbps.push(snap.net.total_rx_rate / 1e6);
        net_tx_mbps.push(snap.net.total_tx_rate / 1e6);
        disk_read_mbps.push(snap.disk.total_read_rate / 1e6);
        disk_write_mbps.push(snap.disk.total_write_rate / 1e6);
    }
};

} // namespace htop_killer::core
