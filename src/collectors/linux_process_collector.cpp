#include "collectors/linux_process_collector.hpp"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <pwd.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

namespace htop_killer::collectors {

namespace {

std::string uid_to_user(uid_t uid) {
    struct passwd* pw = getpwuid(uid);
    return pw ? pw->pw_name : std::to_string(uid);
}

} // namespace

LinuxProcessCollector::LinuxProcessCollector() {
    hz_        = sysconf(_SC_CLK_TCK);
    prev_time_ = core::Clock::now();
}

core::ProcessList LinuxProcessCollector::collect(std::uint64_t total_mem_kb,
                                                  std::uint32_t) {
    const auto now = core::Clock::now();
    const double dt = std::chrono::duration<double>(now - prev_time_).count();

    core::ProcessList list;
    list.timestamp = now;

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return list;

    std::unordered_map<std::uint32_t, ProcSnapshot> curr_snaps;
    struct dirent* entry;

    while ((entry = readdir(proc_dir)) != nullptr) {
        std::uint32_t pid = 0;
        auto [ptr, ec] = std::from_chars(entry->d_name,
                                          entry->d_name + std::strlen(entry->d_name), pid);
        if (ec != std::errc{}) continue;

        const std::string base = "/proc/" + std::to_string(pid);
        std::ifstream stat_f{base + "/stat"};
        if (!stat_f.is_open()) continue;

        std::string stat_line;
        std::getline(stat_f, stat_line);

        const auto comm_end   = stat_line.rfind(')');
        const auto comm_start = stat_line.find('(');
        if (comm_end == std::string::npos) continue;

        std::string comm = stat_line.substr(comm_start + 1, comm_end - comm_start - 1);
        std::istringstream ss{stat_line.substr(comm_end + 2)};

        std::string   state;
        std::uint32_t ppid;
        std::int32_t  pgrp_n, session, tty, tpgid;
        std::uint64_t flags, minflt, cminflt, majflt, cmajflt;
        std::uint64_t utime, stime, cutime, cstime;
        std::int32_t  priority, nice;
        std::uint32_t num_threads;
        std::uint64_t itrealvalue, starttime, vsize;
        std::int64_t  rss;

        ss >> state >> ppid >> pgrp_n >> session >> tty >> tpgid
           >> flags >> minflt >> cminflt >> majflt >> cmajflt
           >> utime >> stime >> cutime >> cstime
           >> priority >> nice >> num_threads
           >> itrealvalue >> starttime >> vsize >> rss;

        curr_snaps[pid] = {utime, stime, cutime, cstime};

        core::ProcessInfo info;
        info.pid        = pid;
        info.ppid       = ppid;
        info.name       = std::move(comm);
        info.state      = std::move(state);
        info.threads    = num_threads;
        info.nice       = nice;
        info.mem_rss_kb = static_cast<std::uint64_t>(rss) * 4;

        if (total_mem_kb > 0)
            info.mem_pct = static_cast<double>(info.mem_rss_kb) / total_mem_kb * 100.0;

        if (prev_snaps_.contains(pid) && dt > 0.0) {
            const auto& p    = prev_snaps_[pid];
            const double ticks = static_cast<double>((utime + stime) - (p.utime + p.stime));
            info.cpu_pct     = ticks / (hz_ * dt) * 100.0;
        }

        std::ifstream status_f{base + "/status"};
        std::string   line;
        while (std::getline(status_f, line)) {
            if (line.starts_with("Uid:")) {
                std::istringstream uid_ss{line.substr(4)};
                uid_t uid;
                uid_ss >> uid;
                info.user = uid_to_user(uid);
                break;
            }
        }

        ++list.total_count;
        if (info.state == "R") ++list.running_count;
        else                    ++list.sleeping_count;

        list.processes.push_back(std::move(info));
    }
    closedir(proc_dir);

    std::ranges::sort(list.processes, [](const auto& a, const auto& b) {
        if (a.cpu_pct != b.cpu_pct) return a.cpu_pct > b.cpu_pct;
        return a.pid < b.pid;
    });

    prev_snaps_ = std::move(curr_snaps);
    prev_time_  = now;
    return list;
}

} // namespace htop_killer::collectors
