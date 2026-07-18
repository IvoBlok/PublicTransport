#include <renderer/renderEngine.hpp>
#include <translation/exampleTranslation.hpp>


int main() {
    renderer::RenderEngine engine;
    engine.initialize();
    auto exampleTranslation = std::make_unique<ExampleTranslation>(engine.getDevice());
    exampleTranslation->testRunSimulation();

    engine.addComputeWrapper(std::move(exampleTranslation));
    
    while(!engine.shouldClose()) {
        engine.handleFrame();
    }

    engine.cleanup();
    return 0;
}

