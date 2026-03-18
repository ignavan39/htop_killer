#include "ui/app.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    try {
        return htop_killer::App{}.run();
    } catch (const std::exception& ex) {
        std::cerr << "htop_killer: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
}
