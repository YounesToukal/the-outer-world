#include <exception>
#include <iostream>

#include "application.h"

int main() {
    try {
        Application application;
        application.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
