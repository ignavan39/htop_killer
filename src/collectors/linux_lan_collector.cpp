#include "collectors/linux_lan_collector.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <charconv>
#include <cstring>
#include <fstream>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace htop_killer::collectors {

//TODO move to a file
//TODO more oui table
static const std::pair<std::string_view, std::string_view> kOuiTable[] = {
    {"00:00:0c", "Cisco"},       {"00:0c:29", "VMware"},      {"00:1a:11", "Google"},
    {"00:1b:21", "Intel"},       {"00:1c:42", "Parallels"},   {"00:26:b9", "Dell"},
    {"00:50:56", "VMware"},       {"00:50:ba", "D-Link"},      {"08:00:27", "VirtualBox"},
    {"0c:84:dc", "Apple"},       {"10:02:b5", "Intel"},       {"18:65:90", "Apple"},
    {"1c:1b:0d", "HP"},          {"1c:69:7a", "ASUSTek"},     {"24:ab:81", "Apple"},
    {"2c:54:cf", "Apple"},       {"34:17:eb", "Apple"},       {"3c:07:54", "Apple"},
    {"44:4c:0c", "Apple"},       {"44:d9:e7", "Intel"},       {"48:5d:60", "Apple"},
    {"54:26:96", "Apple"},       {"5c:f9:38", "Apple"},       {"64:76:ba", "Apple"},
    {"68:96:7b", "Apple"},       {"6c:40:08", "Apple"},       {"74:da:38", "Edimax"},
    {"78:31:c1", "Apple"},       {"84:38:35", "Apple"},       {"90:72:40", "Apple"},
    {"94:94:26", "Asus"},        {"98:01:a7", "Apple"},       {"a4:5e:60", "Apple"},
    {"ac:de:48", "Apple"},       {"b4:fb:e4", "Dell"},        {"b8:27:eb", "Raspberry Pi"},
    {"bc:52:b7", "Apple"},       {"c0:9f:42", "Apple"},       {"c8:2a:14", "Apple"},
    {"c8:b5:b7", "Apple"},       {"d0:23:db", "Apple"},       {"d4:61:9d", "Apple"},
    {"d8:30:62", "Apple"},       {"dc:a9:04", "Apple"},       {"dc:a6:32", "Raspberry Pi"},
    {"e4:5f:01", "Raspberry Pi"},{"e8:04:0b", "Apple"},       {"ec:35:86", "Apple"},
    {"f0:18:98", "Apple"},       {"fc:fc:48", "Apple"},       {"00:11:32", "Synology"},
    {"00:15:5d", "Microsoft"},   {"00:17:88", "Philips Hue"}, {"18:b4:30", "Nest"},
    {"44:61:32", "Google"},       {"f4:f5:d8", "Google"},      {"00:1a:79", "Ubiquiti"},
    {"18:e8:29", "Ubiquiti"},    {"24:a4:3c", "Ubiquiti"},    {"00:0d:3a", "Microsoft"},
    {"28:18:78", "Xiaomi"},      {"34:ce:00", "Xiaomi"},      {"64:09:80", "Xiaomi"},
    {"8c:be:be", "Xiaomi"},      {"f4:8b:32", "Xiaomi"},
};

static std::string lookup_oui(const std::string& mac) {
    if (mac.size() < 8) return "Unknown";
    std::string prefix = mac.substr(0, 8);
    for (auto& c : prefix) c = static_cast<char>(std::tolower(c));
    for (const auto& [oui, vendor] : kOuiTable)
        if (prefix == oui) return std::string(vendor);
    return "Unknown";
}

LinuxLanCollector::LinuxLanCollector() {
    prev_time_ = core::Clock::now();
    gateway_   = read_gateway();
    local_ip_  = read_local_ip();
    iface_     = detect_iface();

    raw_fd_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (raw_fd_ >= 0) {
        if (!iface_.empty()) {
            struct sockaddr_ll sll{};
            sll.sll_family   = AF_PACKET;
            sll.sll_protocol = htons(ETH_P_IP);
            sll.sll_ifindex  = static_cast<int>(if_nametoindex(iface_.c_str()));
            bind(raw_fd_, reinterpret_cast<struct sockaddr*>(&sll), sizeof(sll));
        }
        sniffer_running_.store(true);
        sniffer_thread_ = std::thread([this] { sniffer_loop(iface_); });
    }
}

LinuxLanCollector::~LinuxLanCollector() {
    sniffer_running_.store(false);
    if (raw_fd_ >= 0) {
        shutdown(raw_fd_, SHUT_RDWR);
        close(raw_fd_);
        raw_fd_ = -1;
    }
    if (sniffer_thread_.joinable())
        sniffer_thread_.join();

    std::lock_guard lock{counters_mutex_};
    for (auto& [ip, ptr] : counters_) delete ptr;
}

void LinuxLanCollector::sniffer_loop(const std::string&) {
    alignas(16) char buf[65536];

    while (sniffer_running_.load()) {
        const ssize_t n = recv(raw_fd_, buf, sizeof(buf), 0);
        if (n <= 0) break;
        if (n < (ssize_t)sizeof(struct iphdr)) continue;

        if (n < 14 + (ssize_t)sizeof(struct iphdr)) continue;

        const auto* ip_hdr = reinterpret_cast<const struct iphdr*>(buf + 14);
        if (ip_hdr->version != 4) continue;

        char src_buf[INET_ADDRSTRLEN], dst_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip_hdr->saddr, src_buf, sizeof(src_buf));
        inet_ntop(AF_INET, &ip_hdr->daddr, dst_buf, sizeof(dst_buf));

        const std::string src{src_buf};
        const std::string dst{dst_buf};
        const auto        pkt_len = static_cast<std::uint64_t>(n - 14);

        // Skip loopback
        if (src.starts_with("127.") || dst.starts_with("127.")) continue;

        std::lock_guard lock{counters_mutex_};

        if (src != local_ip_ && !src.starts_with("0.")) {
            if (!counters_.contains(src)) counters_[src] = new HostCounters{};
            counters_[src]->tx.fetch_add(pkt_len, std::memory_order_relaxed);
        }
        if (dst != local_ip_ && !dst.starts_with("0.") && !dst.starts_with("255.")) {
            if (!counters_.contains(dst)) counters_[dst] = new HostCounters{};
            counters_[dst]->rx.fetch_add(pkt_len, std::memory_order_relaxed);
        }
    }
}

core::LanStats LinuxLanCollector::collect() {
    const auto   now = core::Clock::now();
    const double dt  = std::chrono::duration<double>(now - prev_time_).count();

    core::LanStats stats;
    stats.timestamp     = now;
    stats.sniffer_active = (raw_fd_ >= 0);
    stats.gateway_ip = gateway_;
    stats.local_ip   = local_ip_;

    auto arp       = read_arp_table();
    auto tcp_conns = read_tcp_connections();

    std::unordered_map<std::string, HostSnapshot> curr_bytes;
    {
        std::lock_guard lock{counters_mutex_};
        for (const auto& [ip, ptr] : counters_) {
            curr_bytes[ip] = {
                ptr->rx.load(std::memory_order_relaxed),
                ptr->tx.load(std::memory_order_relaxed)
            };
        }
    }

    curr_bytes[local_ip_] = curr_bytes.count(local_ip_) ? curr_bytes[local_ip_] : HostSnapshot{};

    for (auto& [ip, mac] : arp) {
        core::LanDevice dev;
        dev.ip         = ip;
        dev.mac        = mac;
        dev.iface      = iface_;
        dev.vendor     = lookup_oui(mac);
        dev.is_gateway = (ip == gateway_);
        dev.is_self    = (ip == local_ip_);
        dev.last_seen  = now;

        if (hostname_cache_.contains(ip))
            dev.hostname = hostname_cache_[ip];
        else {
            dev.hostname = resolve_hostname(ip);
            hostname_cache_[ip] = dev.hostname;
        }

        if (curr_bytes.contains(ip)) {
            const auto& snap = curr_bytes[ip];
            dev.rx_bytes = snap.rx_bytes;
            dev.tx_bytes = snap.tx_bytes;

            if (prev_bytes_.contains(ip) && dt > 0.0) {
                const auto& prev = prev_bytes_[ip];
                if (snap.rx_bytes >= prev.rx_bytes)
                    dev.rx_rate = static_cast<double>(snap.rx_bytes - prev.rx_bytes) / dt;
                if (snap.tx_bytes >= prev.tx_bytes)
                    dev.tx_rate = static_cast<double>(snap.tx_bytes - prev.tx_bytes) / dt;
            }
        }

        if (tcp_conns.contains(ip))
            dev.open_ports = tcp_conns[ip];

        stats.devices.push_back(std::move(dev));
    }

    std::sort(stats.devices.begin(), stats.devices.end(), [](const auto& a, const auto& b) {
        if (a.is_gateway != b.is_gateway) return a.is_gateway > b.is_gateway;
        if (a.is_self    != b.is_self)    return a.is_self    > b.is_self;
        const double ra = a.rx_rate + a.tx_rate;
        const double rb = b.rx_rate + b.tx_rate;
        if (ra != rb) return ra > rb;
        return a.ip < b.ip;
    });

    prev_bytes_ = curr_bytes;
    prev_time_  = now;
    return stats;
}

std::unordered_map<std::string, std::string> LinuxLanCollector::read_arp_table() {
    std::unordered_map<std::string, std::string> table;
    std::ifstream f{"/proc/net/arp"};
    std::string line;
    std::getline(f, line);
    while (std::getline(f, line)) {
        std::istringstream ss{line};
        std::string ip, hw_type, flags, mac, mask, iface;
        ss >> ip >> hw_type >> flags >> mac >> mask >> iface;
        if (mac == "00:00:00:00:00:00") continue;
        if (flags == "0x0") continue;
        table[ip] = mac;
    }
    return table;
}

std::string LinuxLanCollector::resolve_hostname(const std::string& ip) {
    {
        std::ifstream hosts{"/etc/hosts"};
        std::string line;
        while (std::getline(hosts, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ss{line};
            std::string addr, name;
            ss >> addr >> name;
            if (addr == ip && !name.empty()) return name;
        }
    }
    char host[NI_MAXHOST] = {};
    struct sockaddr_in sa{};
    sa.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip.c_str(), &sa.sin_addr) == 1) {
        if (getnameinfo(reinterpret_cast<const struct sockaddr*>(&sa),
                        sizeof(sa), host, NI_MAXHOST,
                        nullptr, 0, NI_NAMEREQD) == 0) {
            return host;
        }
    }
    return {};
}

std::string LinuxLanCollector::oui_vendor(const std::string& mac) {
    return lookup_oui(mac);
}

std::string LinuxLanCollector::read_gateway() {
    std::ifstream f{"/proc/net/route"};
    std::string line;
    std::getline(f, line);
    while (std::getline(f, line)) {
        std::istringstream ss{line};
        std::string iface, dest_hex, gw_hex, flags_hex;
        ss >> iface >> dest_hex >> gw_hex >> flags_hex;
        if (dest_hex != "00000000") continue;
        unsigned flags = 0;
        std::from_chars(flags_hex.data(), flags_hex.data() + flags_hex.size(), flags, 16);
        if (!(flags & 0x2)) continue;
        unsigned gw_int = 0;
        std::from_chars(gw_hex.data(), gw_hex.data() + gw_hex.size(), gw_int, 16);
        struct in_addr addr{};
        addr.s_addr = gw_int;
        return inet_ntoa(addr);
    }
    return {};
}

std::string LinuxLanCollector::read_local_ip() {
    std::ifstream f{"/proc/net/fib_trie"};
    std::string line, prev_ip;
    while (std::getline(f, line)) {
        const auto dash = line.find("|-- ");
        if (dash != std::string::npos) {
            prev_ip = line.substr(dash + 4);
            while (!prev_ip.empty() && std::isspace(static_cast<unsigned char>(prev_ip.back())))
                prev_ip.pop_back();
            continue;
        }
        if (line.find("LOCAL") != std::string::npos && !prev_ip.empty()) {
            if (prev_ip.starts_with("127.") || prev_ip == "0.0.0.0") { prev_ip.clear(); continue; }
            return prev_ip;
        }
    }
    return {};
}

std::string LinuxLanCollector::detect_iface() {
    std::ifstream f{"/proc/net/route"};
    std::string line;
    std::getline(f, line);
    while (std::getline(f, line)) {
        std::istringstream ss{line};
        std::string iface, dest, gw, flags_str;
        ss >> iface >> dest >> gw >> flags_str;
        if (dest != "00000000") continue;
        unsigned flags = 0;
        std::from_chars(flags_str.data(), flags_str.data() + flags_str.size(), flags, 16);
        if (flags & 0x2) return iface;
    }
    return {};
}

std::unordered_map<std::string, std::uint32_t>
LinuxLanCollector::read_tcp_connections() {
    std::unordered_map<std::string, std::uint32_t> table;
    for (const auto* path : {"/proc/net/tcp", "/proc/net/tcp6"}) {
        std::ifstream f{path};
        if (!f.is_open()) continue;
        std::string line;
        std::getline(f, line);
        while (std::getline(f, line)) {
            std::istringstream ss{line};
            std::string sl, local, remote, state;
            ss >> sl >> local >> remote >> state;
            if (state != "01") continue;
            if (remote.size() < 8) continue;
            unsigned ip_int = 0;
            std::from_chars(remote.data(), remote.data() + 8, ip_int, 16);
            struct in_addr addr{};
            addr.s_addr = ip_int;
            std::string ip_str = inet_ntoa(addr);
            if (ip_str.starts_with("127.") || ip_str == "0.0.0.0") continue;
            table[ip_str]++;
        }
    }
    return table;
}

} // namespace htop_killer::collectors
