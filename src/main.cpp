#include <iostream>
#include <renderer/renderer.hpp>

int main() {
    RenderEngine renderer;

    try {
        renderer.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

