#include <iostream>
#include <renderer/renderer.hpp>

#include <translation/ExampleTranslation.hpp>

int main() {
    renderer::RenderEngine renderer;
    renderer.initialize();
    
    auto exampleTranslation = std::make_unique<ExampleTranslation>(renderer.getContext());
    exampleTranslation->testRunSimulation();
    renderer.wrappers.push_back(std::move(exampleTranslation));
    
    while(!renderer.shouldWindowClose()) {
        renderer.handleFrame();
    }
    renderer.cleanup();
}

