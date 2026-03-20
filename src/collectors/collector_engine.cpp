#include "collectors/collector_engine.hpp"

namespace htop_killer::collectors {

CollectorEngine::CollectorEngine(core::DataStore& store,
                                 std::chrono::milliseconds interval)
    : store_{store}, interval_{interval} {}

CollectorEngine::~CollectorEngine() {
    stop();
}

void CollectorEngine::start() {
    if (running_.exchange(true)) return;
    thread_ = std::jthread{[this](std::stop_token) { loop(); }};
}

void CollectorEngine::stop() {
    running_.store(false);
    if (thread_.joinable()) thread_.request_stop();
}

void CollectorEngine::loop() {
    // LAN scan is slow (hostname resolution) — run it every 5 seconds
    int lan_tick = 0;
    core::LanStats last_lan;

    while (running_.load()) {
        const auto tick_start = core::Clock::now();

        try {
            core::SystemSnapshot snap;
            snap.timestamp = tick_start;
            snap.cpu       = cpu_.collect();
            snap.mem       = mem_.collect();
            snap.net       = net_.collect();
            snap.disk      = disk_.collect();
            snap.procs     = procs_.collect(snap.mem.total_kb, snap.cpu.core_count);

            if (lan_tick == 0) {
                last_lan = lan_.collect();
            }
            snap.lan = last_lan;
            lan_tick = (lan_tick + 1) % 5;

            store_.update(std::move(snap));
        } catch (...) {}

        const auto elapsed = core::Clock::now() - tick_start;
        if (elapsed < interval_)
            std::this_thread::sleep_for(interval_ - elapsed);
    }
}

} // namespace htop_killer::collectors
