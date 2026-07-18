#include <renderer/renderEngine.hpp>
#include <renderer/settings.hpp>

#include <tracy/Tracy.hpp>

#include <iostream>
#include <stdexcept>
#include <cstring>

namespace renderer {

    // ========================================================================
    // Construction / Destruction
    // ========================================================================

    RenderEngine::RenderEngine() {
        initWindow();
    }

    RenderEngine::~RenderEngine() {
        cleanup();
    }

    // ========================================================================
    // Initialisation
    // ========================================================================

    void RenderEngine::initialize() {
        ZoneScoped;

        device = std::make_unique<Device>(enableValidationLayers, window);

        renderPass = std::make_unique<RenderPass>(*device, device->chooseSurfaceFormat().format);

        swapChain = std::make_unique<SwapChain>(*device, *renderPass, window);

        frameManager = std::make_unique<FrameManager>(*device, *renderPass, MAX_FRAMES_IN_FLIGHT);
        frameManager->setSwapchainImageCount(static_cast<uint32_t>(swapChain->getImages().size()));

        pipelineManager = std::make_unique<PipelineManager>(*device, *renderPass,
                                                            frameManager->getDescriptorSetLayout());

        lastFrameTime = std::chrono::high_resolution_clock::now();
    }

    // ========================================================================
    // Main loop
    // ========================================================================

    void RenderEngine::handleFrame() {
        ZoneScoped;

        glfwPollEvents();
        handleInput();

        // --------------------------------------------------------------------
        // Collect all renderables
        // --------------------------------------------------------------------
        std::vector<renderer::Renderable*> allRenderables;
        allRenderables.reserve(renderables.size() + wrapperRenderables.size());

        for (auto& r : renderables) allRenderables.push_back(r.get());
        for (auto* r : wrapperRenderables) allRenderables.push_back(r);

        // --------------------------------------------------------------------
        // Update all renderables
        // --------------------------------------------------------------------
        for (auto& renderable : allRenderables) {
            renderable->updateGPU(*device);
        }

        // --------------------------------------------------------------------
        // SwapChain recreation if needed
        // --------------------------------------------------------------------
        if (swapChain->isInvalid()) {
            recreateSwapChain();
            return;
        }

        // --------------------------------------------------------------------
        // Frame management
        // --------------------------------------------------------------------
        uint32_t frameIndex = frameManager->getNextFrameIndex();
        uint64_t signalValue = frameManager->getNextSignalValue();
        uint64_t waitForID = signalValue > MAX_FRAMES_IN_FLIGHT ? signalValue - MAX_FRAMES_IN_FLIGHT : 0;
        if (waitForID > 0) {
            frameManager->waitForTimelineValue(waitForID);
        }

        // Acquire next swapchain image
        uint32_t imageIndex;
        VkResult acquireResult = swapChain->acquireNextImage(
            imageIndex,
            frameManager->getImageAcquiredSemaphore(frameIndex)
        );

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR) {
            recreateSwapChain();
            return;
        } else if (acquireResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        // Update uniform buffer
        updateUniformBuffer(frameIndex);

        // Record command buffer
        frameManager->recordFrame(
            frameIndex,
            swapChain->getFramebuffers()[imageIndex],
            swapChain->getExtent(),
            allRenderables,
            *pipelineManager
        );


        // Submit info
        // ===============================================
        // Acquire wait semaphore (image acquired)
        VkSemaphoreSubmitInfo acquireWait{};
        acquireWait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        acquireWait.pNext = nullptr;
        acquireWait.semaphore = frameManager->getImageAcquiredSemaphore(frameIndex);
        acquireWait.value = 0;
        acquireWait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
                                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;

        // Command buffer
        VkCommandBufferSubmitInfo cmdInfo{};
        cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmdInfo.commandBuffer = frameManager->getCommandBuffer(frameIndex);

        // Signal semaphores: render complete (binary) + timeline
        VkSemaphore renderComplete = frameManager->getRenderCompleteSemaphore(imageIndex);
        VkSemaphore timeline = frameManager->getTimelineSemaphore();

        VkSemaphoreSubmitInfo signalInfos[2];
        signalInfos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalInfos[0].pNext = nullptr;
        signalInfos[0].semaphore = renderComplete;
        signalInfos[0].value = 0;
        signalInfos[0].stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

        signalInfos[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalInfos[1].pNext = nullptr;
        signalInfos[1].semaphore = timeline;
        signalInfos[1].value = signalValue;
        signalInfos[1].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &acquireWait;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &cmdInfo;
        submitInfo.signalSemaphoreInfoCount = 2;
        submitInfo.pSignalSemaphoreInfos = signalInfos;

        VkResult submitResult = vkQueueSubmit2(device->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        if (submitResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit command buffer!");
        }

        // Tracy collect (if enabled)
        #ifdef TRACY_ENABLE
        // TracyVkCollectHost(tracyContext); //TODO: we don't have tracy context in this new design yet
        #endif

        // Present
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderComplete;
        presentInfo.swapchainCount = 1;
        VkSwapchainKHR swapchainHandle = swapChain->getSwapchain();
        presentInfo.pSwapchains = &swapchainHandle;
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            swapChain->markInvalid();
        } else if (presentResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        FrameMark;
    }

    // ========================================================================
    // Cleanup
    // ========================================================================

    void RenderEngine::cleanup() {
        if (device) {
            vkDeviceWaitIdle(device->getDevice());
        }

        renderables.clear();
        computeWrappers.clear();

        pipelineManager.reset();
        frameManager.reset();
        swapChain.reset();
        renderPass.reset();
        device.reset();

        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
            window = nullptr;
        }
    }

    bool RenderEngine::shouldClose() const {
        return glfwWindowShouldClose(window);
    }

    // ========================================================================
    // Adding objects
    // ========================================================================

    void RenderEngine::addRenderable(std::unique_ptr<Renderable> renderable) {
        renderables.push_back(std::move(renderable));
    }

    void RenderEngine::addComputeWrapper(std::unique_ptr<ComputeWrapperBase> wrapper) {
        computeWrappers.push_back(std::move(wrapper));
        wrapperRenderables.push_back(computeWrappers.back()->getRenderable());
    }

    // ========================================================================
    // Private helpers
    // ========================================================================

    void RenderEngine::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Renderer", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void RenderEngine::handleInput() {
        // Compute delta time
        auto now = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrameTime);
        lastFrameTime = now;

        float timeStep = deltaTime.count() * 1e-6f;

        // Camera vectors
        glm::vec3 worldUp = glm::vec3{0.f, 0.f, 1.f};
        glm::vec3 cameraForward = glm::normalize(glm::cross(worldUp, cameraRight));
        glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));

        float velocity = DEFAULT_CAMERA_MOVE_VELOCITY;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) velocity *= 5.f;
        if (glfwGetKey(window, GLFW_KEY_CAPS_LOCK) == GLFW_PRESS) velocity *= 0.2f;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPosition.z += timeStep * velocity;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            cameraPosition.z -= timeStep * velocity;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPosition -= timeStep * velocity * cameraRight;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPosition += timeStep * velocity * cameraRight;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPosition += timeStep * velocity * cameraForward;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPosition -= timeStep * velocity * cameraForward;

        float rotVel = DEFAULT_CAMERA_ROTATE_VELOCITY;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cameraFront = glm::normalize(glm::rotate(cameraFront, timeStep * rotVel, cameraRight));
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraFront = glm::normalize(glm::rotate(cameraFront, -timeStep * rotVel, cameraRight));
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            cameraRight = glm::normalize(glm::rotate(cameraRight, timeStep * rotVel, worldUp));
            cameraFront = glm::normalize(glm::cross(cameraUp, cameraRight));
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            cameraRight = glm::normalize(glm::rotate(cameraRight, -timeStep * rotVel, worldUp));
            cameraFront = glm::normalize(glm::cross(cameraUp, cameraRight));
        }
    }

    void RenderEngine::updateUniformBuffer(uint32_t frameIndex) {
        UniformBufferObject ubo{};
        ubo.model = glm::mat4(1.0f);
        ubo.view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, glm::vec3{0.f, 0.f, 1.f});
        ubo.proj = glm::perspective(glm::radians(45.0f),
                                    swapChain->getExtent().width / (float)swapChain->getExtent().height,
                                    NEAR_PLANE, FAR_PLANE);
        ubo.proj[1][1] *= -1; // Vulkan clip space correction
        frameManager->updateUniformBuffer(frameIndex, ubo);
    }

    void RenderEngine::recreateSwapChain() {
        vkDeviceWaitIdle(device->getDevice());

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        swapChain->recreate();
        frameManager->setSwapchainImageCount(static_cast<uint32_t>(swapChain->getImages().size()));
        swapChain->markValid();
    }

    void RenderEngine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto self = static_cast<RenderEngine*>(glfwGetWindowUserPointer(window));
        if (self && self->swapChain) {
            self->swapChain->markInvalid();
        }
    }

}