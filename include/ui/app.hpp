#pragma once

#include "core/data_store.hpp"
#include "collectors/collector_engine.hpp"
#include "ui/cpu_panel.hpp"
#include "ui/mem_panel.hpp"
#include "ui/net_disk_panel.hpp"
#include "ui/process_panel.hpp"
#include "ui/lan_panel.hpp"
#include "ui/header_widget.hpp"
#include "ui/help_overlay.hpp"
#include "ui/theme.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <atomic>
#include <chrono>
#include <thread>

namespace htop_killer {

class App {
public:
    App() : engine_(store_, std::chrono::seconds{1}) {}

    int run() {
        using namespace ftxui;

        engine_.start();

        auto screen = ScreenInteractive::Fullscreen();

        int  active_tab  = 0;
        bool per_core    = false;
        bool show_help   = false;
        int  proc_scroll = 0;
        int  lan_scroll  = 0;
        auto sort_key    = ui::ProcSortKey::Cpu;

        std::atomic_bool quit{false};
        std::thread refresh_thread([&] {
            while (!quit.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds{500});
                screen.PostEvent(Event::Custom);
            }
        });

        const std::vector<std::string> tabs{
            "  CPU  ","  MEM  ","  NET  "," DISK  "," PROCS "," LAN   "
        };

        auto renderer = Renderer([&]() -> Element {
            auto snap = store_.snapshot();
            auto hist = store_.history();

            Elements tab_elems;
            for (int i = 0; i < (int)tabs.size(); ++i) {
                bool active = (i == active_tab);
                auto t = text(tabs[i]);
                tab_elems.push_back(
                    active ? t | bold | color(Color::Black) | bgcolor(ui::theme::kCpuPrimary)
                           : t | color(ui::theme::kDimText));
                tab_elems.push_back(text("│") | color(ui::theme::kDimText));
            }

            Element content;
            switch (active_tab) {
                case 0: content = ui::make_cpu_panel(snap.cpu, hist, per_core);                          break;
                case 1: content = ui::make_mem_panel(snap.mem, hist);                                    break;
                case 2: content = ui::make_net_panel(snap.net, hist);                                    break;
                case 3: content = ui::make_disk_panel(snap.disk, hist);                                  break;
                case 4: content = ui::make_process_panel(snap.procs, sort_key, 25, proc_scroll);         break;
                case 5: content = ui::make_lan_panel(snap.lan);                                          break;
                default: content = text("?");
            }

            constexpr std::array sort_names{"CPU","MEM","PID","Name"};
            auto status = hbox(
                text("  [?] help  [Tab] next  [c] cores  [f] sort:") | color(ui::theme::kDimText),
                text(sort_names[static_cast<int>(sort_key)]) | bold | color(ui::theme::kCpuPrimary),
                text("  [q] quit") | color(ui::theme::kDimText),
                text("  ")
            ) | bgcolor(ui::theme::kHeaderBg);

            auto base = vbox(
                ui::make_header(snap),
                hbox(std::move(tab_elems)),
                separator() | color(ui::theme::kDimText),
                content | flex,
                status
            );

            if (show_help)
                return dbox(Elements{base, ui::make_help_overlay() | center});

            return base;
        });

        auto component = CatchEvent(renderer, [&](Event e) -> bool {
            if (e == Event::Custom) return false;

            if (e == Event::Character('q') || e == Event::Escape) {
                quit.store(true);
                screen.ExitLoopClosure()();
                return true;
            }
            if (e == Event::Character('?'))  { show_help = !show_help; return true; }
            if (e == Event::Tab)             { active_tab = (active_tab + 1) % (int)tabs.size(); return true; }
            if (e == Event::Character('1'))  { active_tab = 0; return true; }
            if (e == Event::Character('2'))  { active_tab = 1; return true; }
            if (e == Event::Character('3'))  { active_tab = 2; return true; }
            if (e == Event::Character('4'))  { active_tab = 3; return true; }
            if (e == Event::Character('5'))  { active_tab = 4; return true; }  // PROCS
            if (e == Event::Character('6'))  { active_tab = 5; return true; }  // LAN
            if (e == Event::Character('p'))  { active_tab = 4; return true; }
            if (e == Event::Character('l'))  { active_tab = 5; return true; }
            if (e == Event::Character('c'))  { per_core = !per_core; return true; }
            if (e == Event::Character('f') || e == Event::Character('F')) {
                sort_key = static_cast<ui::ProcSortKey>((static_cast<int>(sort_key) + 1) % 4);
                return true;
            }
            if (e == Event::ArrowDown || e == Event::Character('j')) {
                if (active_tab == 4) ++proc_scroll;
                else if (active_tab == 5) ++lan_scroll;
                return true;
            }
            if (e == Event::ArrowUp || e == Event::Character('k')) {
                if (active_tab == 4 && proc_scroll > 0) --proc_scroll;
                else if (active_tab == 5 && lan_scroll > 0) --lan_scroll;
                return true;
            }

            return false;
        });

        screen.Loop(component);

        quit.store(true);
        engine_.stop();
        refresh_thread.join();
        return 0;
    }

private:
    core::DataStore             store_;
    collectors::CollectorEngine engine_;
};

} // namespace htop_killer
