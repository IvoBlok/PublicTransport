#include <renderer/frameManager.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/renderables/renderable.hpp>

#include <tracy/Tracy.hpp>

#include <stdexcept>
#include <cstring>

namespace renderer {

    // ========================================================================
    // Construction / Destruction
    // ========================================================================

    FrameManager::FrameManager(Device& dev, RenderPass& rp, uint32_t maxFrames)
        : device(dev), renderPass(rp), maxFrames(maxFrames) {

        createDescriptorSetLayout();
        createDescriptorPool();
        createPerFrameResources();
        allocateDescriptorSets();
        createUniformBuffers();
        createTimelineSemaphore();
        createDummyPipelineLayout();
    }

    FrameManager::~FrameManager() {
        destroyRenderCompleteSemaphores();
        destroyPerFrameResources();
        destroyDummyPipelineLayout();

        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device.getDevice(), descriptorPool, nullptr);
        }
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device.getDevice(), descriptorSetLayout, nullptr);
        }
        if (timelineSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device.getDevice(), timelineSemaphore, nullptr);
        }
    }

    FrameManager::FrameManager(FrameManager&& other) noexcept = default;

    // ========================================================================
    // Private resource creation (called from constructor)
    // ========================================================================

    void FrameManager::createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboBinding;

        if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
    }

    void FrameManager::createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = maxFrames;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = maxFrames;

        if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    void FrameManager::createPerFrameResources() {
        perFrame.resize(maxFrames);

        for (uint32_t i = 0; i < maxFrames; ++i) {
            auto& data = perFrame[i];

            // Command pool
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = device.getGraphicsQueueFamily();
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            if (vkCreateCommandPool(device.getDevice(), &poolInfo, nullptr, &data.commandPool) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create command pool for frame!");
            }

            // Command buffer
            VkCommandBufferAllocateInfo cmdAlloc{};
            cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdAlloc.commandPool = data.commandPool;
            cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdAlloc.commandBufferCount = 1;
            if (vkAllocateCommandBuffers(device.getDevice(), &cmdAlloc, &data.commandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate command buffer!");
            }

            // Image acquired semaphore (binary)
            VkSemaphoreCreateInfo semInfo{};
            semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            if (vkCreateSemaphore(device.getDevice(), &semInfo, nullptr, &data.imageAcquiredSemaphore) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image acquired semaphore!");
            }
        }
    }

    void FrameManager::allocateDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(maxFrames, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = maxFrames;
        allocInfo.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> sets(maxFrames);
        if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        for (uint32_t i = 0; i < maxFrames; ++i) {
            perFrame[i].descriptorSet = sets[i];
        }
    }

    void FrameManager::createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        for (uint32_t i = 0; i < maxFrames; ++i) {
            auto& data = perFrame[i];

            // Create buffer
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device.getDevice(), &bufferInfo, nullptr, &data.uniformBuffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create uniform buffer!");
            }

            VkMemoryRequirements memReqs;
            vkGetBufferMemoryRequirements(device.getDevice(), data.uniformBuffer, &memReqs);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReqs.size;
            allocInfo.memoryTypeIndex = device.findMemoryType(
                memReqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            if (vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &data.uniformBufferMemory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate uniform buffer memory!");
            }

            vkBindBufferMemory(device.getDevice(), data.uniformBuffer, data.uniformBufferMemory, 0);
            vkMapMemory(device.getDevice(), data.uniformBufferMemory, 0, bufferSize, 0, &data.uniformBufferMapped);

            // Update the descriptor set
            VkDescriptorBufferInfo bufferInfoDesc{};
            bufferInfoDesc.buffer = data.uniformBuffer;
            bufferInfoDesc.offset = 0;
            bufferInfoDesc.range = bufferSize;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = data.descriptorSet;
            write.dstBinding = 0;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &bufferInfoDesc;

            vkUpdateDescriptorSets(device.getDevice(), 1, &write, 0, nullptr);
        }
    }

    void FrameManager::createTimelineSemaphore() {
        VkSemaphoreTypeCreateInfo typeInfo{};
        typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        typeInfo.initialValue = 0;

        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semInfo.pNext = &typeInfo;

        if (vkCreateSemaphore(device.getDevice(), &semInfo, nullptr, &timelineSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create timeline semaphore!");
        }
    }

    void FrameManager::createDummyPipelineLayout() {
        // This layout is only used to bind the descriptor set during command recording.
        // It uses the exact same descriptor set layout as the real pipelines.
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device.getDevice(), &layoutInfo, nullptr, &dummyLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create dummy pipeline layout!");
        }
    }

    void FrameManager::createRenderCompleteSemaphores(uint32_t count) {
        destroyRenderCompleteSemaphores();
        renderCompleteSemaphores.resize(count);
        for (uint32_t i = 0; i < count; ++i) {
            VkSemaphoreCreateInfo semInfo{};
            semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            if (vkCreateSemaphore(device.getDevice(), &semInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create render complete semaphore!");
            }
        }
    }

    // ========================================================================
    // Private resource destruction
    // ========================================================================

    void FrameManager::destroyPerFrameResources() {
        for (auto& data : perFrame) {
            if (data.commandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(device.getDevice(), data.commandPool, nullptr);
            }
            if (data.imageAcquiredSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(device.getDevice(), data.imageAcquiredSemaphore, nullptr);
            }
            if (data.uniformBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device.getDevice(), data.uniformBuffer, nullptr);
            }
            if (data.uniformBufferMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device.getDevice(), data.uniformBufferMemory, nullptr);
            }
            // mapped memory is freed with the memory
        }
        perFrame.clear();
    }

    void FrameManager::destroyRenderCompleteSemaphores() {
        for (auto sem : renderCompleteSemaphores) {
            if (sem != VK_NULL_HANDLE)
                vkDestroySemaphore(device.getDevice(), sem, nullptr);
        }
        renderCompleteSemaphores.clear();
    }

    void FrameManager::destroyDummyPipelineLayout() {
        if (dummyLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device.getDevice(), dummyLayout, nullptr);
            dummyLayout = VK_NULL_HANDLE;
        }
    }

    // ========================================================================
    // Public methods
    // ========================================================================

    void FrameManager::setSwapchainImageCount(uint32_t count) {
        createRenderCompleteSemaphores(count);
    }

    void FrameManager::updateUniformBuffer(uint32_t frameIndex, const UniformBufferObject& ubo) {
        auto& data = perFrame[frameIndex];
        memcpy(data.uniformBufferMapped, &ubo, sizeof(ubo));
    }

    void FrameManager::recordFrame(uint32_t frameIndex,
                                   VkFramebuffer framebuffer,
                                   VkExtent2D extent,
                                   const std::vector<Renderable*>& renderables,
                                   PipelineManager& pipelineManager) {
        ZoneScoped;

        auto& data = perFrame[frameIndex];

        // Reset command pool and begin recording
        vkResetCommandPool(device.getDevice(), data.commandPool, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(data.commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin command buffer!");
        }

        // Begin the render pass
        VkClearValue clearColor = { {0.7f, 0.7f, 0.7f, 1.0f} };
        VkRenderPassBeginInfo rpBegin{};
        rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass = renderPass.get();
        rpBegin.framebuffer = framebuffer;
        rpBegin.renderArea.offset = {0, 0};
        rpBegin.renderArea.extent = extent;
        rpBegin.clearValueCount = 1;
        rpBegin.pClearValues = &clearColor;

        vkCmdBeginRenderPass(data.commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

        // Set viewport and scissor (dynamic)
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(data.commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;
        vkCmdSetScissor(data.commandBuffer, 0, 1, &scissor);

        // Bind the UBO descriptor set once using the dummy layout
        // This binding is valid for the whole render pass because all pipelines share the same layout.
        vkCmdBindDescriptorSets(data.commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                dummyLayout,
                                0, 1, &data.descriptorSet,
                                0, nullptr);

        // Draw all renderables
        for (Renderable* renderable : renderables) {
            const PipelineKey& key = renderable->getPipelineKey();
            const Pipeline& pipeline = pipelineManager.getPipeline(key);

            vkCmdBindPipeline(data.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get());
            renderable->render(data.commandBuffer, pipeline.getLayout(), pipeline.get());
        }

        // End render pass and command buffer
        vkCmdEndRenderPass(data.commandBuffer);

        if (vkEndCommandBuffer(data.commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to end command buffer!");
        }
    }

    // ------------------------------------------------------------------------
    // Getters
    // ------------------------------------------------------------------------

    VkCommandBuffer FrameManager::getCommandBuffer(uint32_t frameIndex) const {
        return perFrame[frameIndex].commandBuffer;
    }

    VkSemaphore FrameManager::getImageAcquiredSemaphore(uint32_t frameIndex) const {
        return perFrame[frameIndex].imageAcquiredSemaphore;
    }

    VkSemaphore FrameManager::getRenderCompleteSemaphore(uint32_t imageIndex) const {
        return renderCompleteSemaphores[imageIndex];
    }

    VkDescriptorSet FrameManager::getDescriptorSet(uint32_t frameIndex) const {
        return perFrame[frameIndex].descriptorSet;
    }

    void FrameManager::waitForTimelineValue(uint64_t value) const {
        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &timelineSemaphore;
        waitInfo.pValues = &value;
        vkWaitSemaphores(device.getDevice(), &waitInfo, UINT64_MAX);
    }

}