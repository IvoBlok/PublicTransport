#pragma once

#include <renderer/settings.hpp>
#include <renderer/device.hpp>
#include <renderer/renderPass.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/renderables/renderable.hpp>

#include <vector>
#include <array>

namespace renderer {

    /**
     * Manages per‑frame resources and command buffer recording.
     *
     * Responsibilities:
     *   - Create/destroy per‑frame command pools, command buffers,
     *     uniform buffers, descriptor sets, and semaphores.
     *   - Record the command buffer for a given frame.
     *   - Provide a shared descriptor‑set layout.
     *   - Handle timeline semaphore control.
     */
    class FrameManager {
    public:
        FrameManager(Device& device, RenderPass& renderPass, uint32_t maxFrames);
        ~FrameManager();

        // Non‑copyable, movable
        FrameManager(const FrameManager&) = delete;
        FrameManager& operator=(const FrameManager&) = delete;
        FrameManager(FrameManager&& other) noexcept;
        FrameManager& operator=(FrameManager&& other) = delete;

        // --------------------------------------------------------------------
        // Public API
        // --------------------------------------------------------------------

        void setSwapchainImageCount(uint32_t count);
        void updateUniformBuffer(uint32_t frameIndex, const UniformBufferObject& ubo);

        void recordFrame(uint32_t frameIndex,
                         VkFramebuffer framebuffer,
                         VkExtent2D extent,
                         const std::vector<Renderable*>& renderables,
                         PipelineManager& pipelineManager);

        // Getters for submission
        VkCommandBuffer getCommandBuffer(uint32_t frameIndex) const;
        VkSemaphore getImageAcquiredSemaphore(uint32_t frameIndex) const;
        VkSemaphore getRenderCompleteSemaphore(uint32_t imageIndex) const;
        VkSemaphore getTimelineSemaphore() const { return timelineSemaphore; }
        VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

        // Frame counting and timeline control
        uint32_t getNextFrameIndex() { return (currentFrame++) % maxFrames; }
        uint64_t getNextSignalValue() { return ++nextSignalValue; }
        void waitForTimelineValue(uint64_t value) const;

    private:
        // --------------------------------------------------------------------
        // Internal data and helpers
        // --------------------------------------------------------------------

        struct PerFrameData {
            VkCommandPool commandPool = VK_NULL_HANDLE;
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
            VkSemaphore imageAcquiredSemaphore = VK_NULL_HANDLE;
            VkBuffer uniformBuffer = VK_NULL_HANDLE;
            VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
            void* uniformBufferMapped = nullptr;
            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        };

        Device& device;
        RenderPass& renderPass;
        uint32_t maxFrames;

        std::vector<PerFrameData> perFrame;
        std::vector<VkSemaphore> renderCompleteSemaphores;

        // Shared resources
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkSemaphore timelineSemaphore = VK_NULL_HANDLE;

        // Dummy pipeline layout just for binding descriptor sets in recordFrame
        VkPipelineLayout dummyLayout = VK_NULL_HANDLE;

        uint64_t currentFrame = 0;
        uint64_t nextSignalValue = 0;

        // Private resource creation/destruction
        void createDescriptorSetLayout();
        void createDescriptorPool();
        void createPerFrameResources();
        void allocateDescriptorSets();
        void createUniformBuffers();
        void createTimelineSemaphore();
        void createDummyPipelineLayout();
        void createRenderCompleteSemaphores(uint32_t count);

        void destroyPerFrameResources();
        void destroyRenderCompleteSemaphores();
        void destroyDummyPipelineLayout();
    };

} // namespace renderer