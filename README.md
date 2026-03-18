# htop_killer

System monitor with live graphs. Reads `/proc` directly, renders in the terminal.

```
  ◈ htop_killer  Intel Core i9-13900K       up 3d 14h  load 1.42 0.89 0.71
  CPU    │   MEM   │   NET   │  DISK  │  PROCS
 ─────────────────────────────────────────────────────────────────────
 CPU  16 cores                                    34.2%   peak: 91.0%
 ────────────────────────────────────────────────────────────────────
       ▄▄                  ▄▄▄▄▄
    ▄▄████▄         ▄▄▄▄▄███████▄▄▄
 ▄▄▄███████████▄▄▄▄██████████████████▄▄▄▄▄▄▄▄▄▄▄████████▄▄
```

## Requirements

- Linux (reads `/proc`)
- CMake ≥ 3.20
- GCC 13+ or Clang 16+ (C++23)
- Git

Dependencies ([FTXUI](https://github.com/ArthurSonzogni/FTXUI), [{fmt}](https://github.com/fmtlib/fmt)) are fetched automatically.

## Build

```bash
git clone https://github.com/yourname/htop_killer
cd htop_killer
./build.sh run
```

Other targets:

```bash
./build.sh release   # build only
./build.sh debug     # with ASan + UBSan
./build.sh install   # to /usr/local/bin
./build.sh clean
./build.sh lint      # clang-tidy
```

Manual:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build -j$(nproc)
./build/htop_killer
```

Docker:

```bash
docker build -t htop_killer .
docker run -it --pid=host --privileged htop_killer
```

## Keys

| Key | Action |
|-----|--------|
| `Tab` | next tab |
| `1` `2` `3` `4` | CPU / MEM / NET / DISK |
| `p` | process table |
| `c` | per-core CPU bars |
| `f` | cycle sort (CPU → MEM → PID → Name) |
| `j` `k` / `↓` `↑` | scroll processes |
| `?` | help overlay |
| `q` `Esc` | quit |

## Structure

```
include/
  core/        types, ring_buffer, history, data_store
  collectors/  interfaces + Linux implementations
  ui/          widgets, theme, app loop
src/
  collectors/  .cpp implementations
  main.cpp
```

Data flows one way: collectors write to `DataStore`, UI reads from it. The shared state is guarded by `std::shared_mutex`. History is stored in fixed-size ring buffers (120 samples ≈ 2 min at 1 Hz).

## License

MIT
