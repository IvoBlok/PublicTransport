#include <renderer/objectTypes/line.hpp>

#include <cstring>

namespace renderer
{
    Line::Line(VulkanContext& renderContext) : renderContext(renderContext) {
        vertices = std::vector<RendererVertex>();
        for (int i = 0; i < 10; i++)
            vertices.push_back(RendererVertex{.pos = glm::ballRand(1.0f)});

        createVertexBuffer();
    }

    Line::~Line() {
        destroy();
    }

    Line::Line(Line&& other) noexcept : 
        renderContext(other.renderContext),
        vertices(std::move(other.vertices)),
        vertexBuffer(other.vertexBuffer),
        vertexBufferMemory(other.vertexBufferMemory)
    {
        other.vertexBuffer = VK_NULL_HANDLE;
        other.vertexBufferMemory = VK_NULL_HANDLE;
    }

    Line& Line::operator=(Line&& other) noexcept {
        if (this != &other) {
            vkDeviceWaitIdle(renderContext.device);
            destroy();

            vertices = std::move(other.vertices);
            vertexBuffer = other.vertexBuffer;
            vertexBufferMemory = other.vertexBufferMemory;

            other.vertexBuffer = VK_NULL_HANDLE;
            other.vertexBufferMemory = VK_NULL_HANDLE;
        }
        return *this;
    }

    void Line::createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

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

    void Line::destroy() {
        if (vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(renderContext.device, vertexBuffer, nullptr);
            vertexBuffer = VK_NULL_HANDLE;
        }
        if (vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(renderContext.device, vertexBufferMemory, nullptr);
            vertexBufferMemory = VK_NULL_HANDLE;
        }
        vertices.clear();
    }

    void Line::render(VkCommandBuffer commandBuffer) const {
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
    }
} // namespace renderer
