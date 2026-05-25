#include <iostream>
#include <renderer/renderer.hpp>

int main() {
    renderer::RenderEngine renderer;

    renderer.initialize();
    
    while(!renderer.shouldWindowClose()) {
        renderer.handleFrame();
    }
    renderer.cleanup();
}

