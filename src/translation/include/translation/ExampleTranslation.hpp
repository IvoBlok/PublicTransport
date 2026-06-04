#pragma once

#include <renderer/coreTypes.hpp>
#include <translation/ComputeWrapperBase.hpp>
#include <compute/example.hpp>

/*
Example of how the translation 'wrapper' around a given simulation might look like; It derives from the base type, holds the compute / simulation class, and handles the input and outputs
In a proper example, this would probably be split into header and src file
*/
class ExampleTranslation : public ComputeWrapperBase {
public:
    ExampleTranslation(renderer::VulkanContext& renderContext) : renderContext(renderContext) {
        exampleSim = compute::ExampleSimulation();
        cachedParams = exampleSim.getParameters();
    }

    void renderGUI() override {
        /*
        ImGui::Begin("name")

        // Parameters section
        if (ImGui::CollapsingHeader("Parameters")) {
            if (ImGui::SliderFloat("Viscosity", &cachedParams.viscosity, 0.0f, 0.1f))
                dirtyParams = true;
            if (ImGui::SliderFloat("Density", &cachedParams.density, 200.0f, 1000.0f))
                dirtyParams = true;
            if (ImGui::SliderFloat("Grid Size", &cachedParams.gridSize, 0, 50))
                dirtyParams = true;
        }

        // Metrics section
        if (ImGui::CollapsingHeader("Metrics")) {
            ImGui::Text("Kinetic Energy: %.4f", cachedState.totalKineticEnergy);
            ImGui::Text("Iteration: %d", cachedState.iteration);
        }

        // Control section
        if (ImGui::Buttom("Run!")) {
            // create RunParameters struct, and fill it in via GUI
            if (dirtyParams) {
                exampleSim.setParameters(cachedParams);
                dirtyParams = false;
            }
            exampleSim.run(runParams, onComputeStateUpdate);
        }

        ImGui::End();
        */
    }

    std::vector<renderer::Line>& getLines() override { return renderLines; }
    
    void testRunSimulation() {
        exampleSim.setParameters(cachedParams);
        exampleSim.run(compute::ExampleSimulation::RunParameters{.numSteps = 100}, [this](const compute::ExampleSimulation::State& state) { this->onComputeStateUpdate(state); });
    }

private:
    compute::ExampleSimulation exampleSim;
    compute::ExampleSimulation::State cachedState;
    compute::ExampleSimulation::Parameters cachedParams;

    renderer::VulkanContext& renderContext;

    // TODO: renderer:Line already needs to be reworked to LineSet, but because that's not yet the case we have another issue here; we could be pushing/popping elements to this while the renderer is reading them. If we use LineSet, where the full vector is double-buffered, this issue is resolved
    std::vector<renderer::Line> renderLines;

    void onComputeStateUpdate(const compute::ExampleSimulation::State& state) {
        cachedState = state;

        renderLines.clear();
        for (int i = 0; i < cachedState.velocity.size(); i++) {
            if (i >= renderLines.size()) renderLines.push_back(renderer::Line(renderContext));

            auto& vertices = renderLines[i].startWrite();
            for (size_t j = 0; j < 10; j++)
                vertices.push_back(renderer::RendererVertex{.pos = glm::ballRand(1.0f)});
            
            renderLines[i].endWrite();
        }
    }
};