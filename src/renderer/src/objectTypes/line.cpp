#include <renderer/objectTypes/line.hpp>

#include <cstring>

namespace renderer
{
    VkVertexInputBindingDescription LineSetVertex::getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(LineSetVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    std::array<VkVertexInputAttributeDescription, 2> LineSetVertex::getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(LineSetVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(LineSetVertex, color);

        return attributeDescriptions;
    }

    LineSet::LineSet(VulkanContext& renderContext) : renderContext(renderContext) {
        vertexBuffer = VK_NULL_HANDLE;
        vertexBufferMemory = VK_NULL_HANDLE;
    }

    LineSet::~LineSet() {
        destroy();
    }

    LineSet::LineSet(LineSet&& other) noexcept : 
        renderContext(other.renderContext),
        vertices(std::move(other.vertices)),
        vertexBuffer(other.vertexBuffer),
        vertexBufferMemory(other.vertexBufferMemory),
        vertexCount(other.vertexCount)
    {
        other.vertexBuffer = VK_NULL_HANDLE;
        other.vertexBufferMemory = VK_NULL_HANDLE;
        other.vertexCount = 0;
    }

    LineSet& LineSet::operator=(LineSet&& other) noexcept {
        if (this != &other) {
            vkDeviceWaitIdle(renderContext.device);
            destroy();

            vertices = std::move(other.vertices);
            vertexBuffer = other.vertexBuffer;
            vertexBufferMemory = other.vertexBufferMemory;
            vertexCount = other.vertexCount;

            other.vertexBuffer = VK_NULL_HANDLE;
            other.vertexBufferMemory = VK_NULL_HANDLE;
            other.vertexCount = 0;
        }
        
        return *this;
    }

    bool LineSet::needsUpdate() const {
        return vertices.hasNewData();
    }

    void LineSet::updateGPU() {
        if (!needsUpdate()) return;

        const auto& front = vertices.swapAndGetFront();
        createRenderBuffers(front);
    }

    void LineSet::createRenderBuffers(const std::vector<LineSetVertex>& vertices) {
        destroy();

        if (vertices.empty()) return;

        vertexCount = vertices.size();
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(renderContext, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(renderContext.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(renderContext.device, stagingBufferMemory);

        createBuffer(renderContext, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        copyBuffer(renderContext, stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(renderContext.device, stagingBuffer, nullptr);
        vkFreeMemory(renderContext.device, stagingBufferMemory, nullptr);
    }

    void LineSet::destroy() {
        if (vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(renderContext.device, vertexBuffer, nullptr);
            vertexBuffer = VK_NULL_HANDLE;
        }
        if (vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(renderContext.device, vertexBufferMemory, nullptr);
            vertexBufferMemory = VK_NULL_HANDLE;
        }
        vertexCount = 0;
    }

    void LineSet::render(VkCommandBuffer commandBuffer) const {
        if (vertexBuffer == VK_NULL_HANDLE || vertexCount == 0) return;

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertexCount), 1, 0, 0);
    }
} // namespace renderer
