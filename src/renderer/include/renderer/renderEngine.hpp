#pragma once

#include <renderer/settings.hpp>
#include <renderer/device.hpp>
#include <renderer/renderPass.hpp>
#include <renderer/swapChain.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/frameManager.hpp>
#include <renderer/renderables/renderable.hpp>

#include <translation/computeWrapperBase.hpp>

#include <memory>
#include <vector>
#include <chrono>

namespace renderer {

    /**
     * Main engine class. Owns all Vulkan components, runs the render loop.
     */
    class RenderEngine {
    public:
        RenderEngine();
        ~RenderEngine();

        // Initialise window and Vulkan
        void initialize();

        // Process one frame (input, update, render, present)
        void handleFrame();

        // Cleanup all resources
        void cleanup();

        // Check if window should close
        bool shouldClose() const;

        // Add a renderable object (takes ownership)
        void addRenderable(std::unique_ptr<Renderable> renderable);

        // Add a compute wrapper (takes ownership)
        void addComputeWrapper(std::unique_ptr<ComputeWrapperBase> wrapper);

        Device& getDevice() { return *device; }

    private:
        // --------------------------------------------------------------------
        // Core components
        // --------------------------------------------------------------------
        std::unique_ptr<Device> device;
        std::unique_ptr<RenderPass> renderPass;
        std::unique_ptr<SwapChain> swapChain;
        std::unique_ptr<FrameManager> frameManager;
        std::unique_ptr<PipelineManager> pipelineManager;

        GLFWwindow* window = nullptr;

        // Renderables and compute wrappers
        std::vector<std::unique_ptr<Renderable>> renderables;
        std::vector<std::unique_ptr<ComputeWrapperBase>> computeWrappers;
        std::vector<renderer::Renderable*> wrapperRenderables;

        // Camera state
        glm::vec3 cameraPosition = {0.f, -2.f, 0.f};
        glm::vec3 cameraFront = {0.f, 1.f, 0.f};
        glm::vec3 cameraRight = {1.f, 0.f, 0.f};

        // Timing
        std::chrono::high_resolution_clock::time_point lastFrameTime;
        std::chrono::microseconds deltaTime = std::chrono::microseconds(0);

        // Internal helpers
        void initWindow();
        void handleInput();
        void updateUniformBuffer(uint32_t frameIndex);
        void recreateSwapChain();
        void cleanupSwapChain();

        // Static callback for window resize
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    };

}