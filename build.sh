#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${ROOT}/build"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)}"

usage() {
    echo "usage: $0 [clean|debug|release|run|install|lint]"
    exit 1
}

configure() {
    local extra_args=()

    if command -v ninja &>/dev/null; then
        extra_args+=(-G Ninja)
    fi

    cmake -S "$ROOT" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        "${extra_args[@]}"

    ln -sf "${BUILD_DIR}/compile_commands.json" "${ROOT}/compile_commands.json" 2>/dev/null || true
}

build() {
    configure
    cmake --build "$BUILD_DIR" --parallel "$JOBS"
}

CMD="${1:-release}"
case "$CMD" in
    clean)   rm -rf "$BUILD_DIR" ;;
    debug)   BUILD_TYPE=Debug   build ;;
    release) BUILD_TYPE=Release build ;;
    run)
        BUILD_TYPE=Release build
        exec "${BUILD_DIR}/htop_killer"
        ;;
    install)
        BUILD_TYPE=Release build
        cmake --install "$BUILD_DIR" --prefix /usr/local
        ;;
    lint)
        configure
        find "${ROOT}/src" "${ROOT}/include" \( -name '*.cpp' -o -name '*.hpp' \) \
        | xargs clang-tidy -p "${BUILD_DIR}"
        ;;
    *) usage ;;
esac
