#include <renderer/renderables/line.hpp>

#include <cstring>
#include <stdexcept>

namespace renderer {

    // ========================================================================
    // LineSetVertex helpers
    // ========================================================================

    VkVertexInputBindingDescription LineSetVertex::getBindingDescription() {
        VkVertexInputBindingDescription desc{};
        desc.binding = 0;
        desc.stride = sizeof(LineSetVertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    std::array<VkVertexInputAttributeDescription, 2> LineSetVertex::getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attrs{};
        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(LineSetVertex, pos);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(LineSetVertex, color);

        return attrs;
    }

    // ========================================================================
    // LineSet implementation
    // ========================================================================

    LineSet::LineSet(Device& dev) : device(dev) {}

    LineSet::~LineSet() {
        destroyBuffers();
    }

    LineSet::LineSet(LineSet&& other) noexcept
        : device(other.device),
          vertices(std::move(other.vertices)),
          vertexBuffer(other.vertexBuffer),
          vertexBufferMemory(other.vertexBufferMemory),
          vertexCount(other.vertexCount) {
        other.vertexBuffer = VK_NULL_HANDLE;
        other.vertexBufferMemory = VK_NULL_HANDLE;
        other.vertexCount = 0;
    }

    // ------------------------------------------------------------------------
    // Renderable implementation
    // ------------------------------------------------------------------------

    PipelineKey LineSet::getPipelineKey() const {
        PipelineKey key;
        key.vertexShaderPath = "line.vert.spv";
        key.fragmentShaderPath = "line.frag.spv";

        key.binding = LineSetVertex::getBindingDescription();

        auto attrs = LineSetVertex::getAttributeDescriptions();
        key.attributes.assign(attrs.begin(), attrs.end());

        key.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        key.polygonMode = VK_POLYGON_MODE_LINE;
        key.cullMode = VK_CULL_MODE_NONE;
        key.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        key.lineWidth = 1.0f;
        return key;
    }

    bool LineSet::updateGPU(Device& dev) {
        if (!vertices.hasNewData()) return false;

        const auto& front = vertices.swapAndGetFront();
        destroyBuffers();
        if (!front.empty()) {
            createBuffers(dev, front);
        }
        return true;
    }

    void LineSet::render(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline pipeline) const {
        if (vertexBuffer == VK_NULL_HANDLE || vertexCount == 0) return;

        VkBuffer buffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        vkCmdDraw(cmd, static_cast<uint32_t>(vertexCount), 1, 0, 0);
    }

    // ------------------------------------------------------------------------
    // Buffer management
    // ------------------------------------------------------------------------

    void LineSet::destroyBuffers() {
        if (vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device.getDevice(), vertexBuffer, nullptr);
            vertexBuffer = VK_NULL_HANDLE;
        }
        if (vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device.getDevice(), vertexBufferMemory, nullptr);
            vertexBufferMemory = VK_NULL_HANDLE;
        }
        vertexCount = 0;
    }

    void LineSet::createBuffers(Device& dev, const std::vector<LineSetVertex>& data) {
        if (data.empty()) return;

        vertexCount = data.size();
        VkDeviceSize bufferSize = sizeof(LineSetVertex) * vertexCount;

        // Staging buffer (host‑visible)
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        stagingBuffer = dev.createBuffer(bufferSize,
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         stagingMemory);

        // Copy data to staging
        void* mapped;
        vkMapMemory(dev.getDevice(), stagingMemory, 0, bufferSize, 0, &mapped);
        memcpy(mapped, data.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(dev.getDevice(), stagingMemory);

        // Device‑local vertex buffer
        vertexBuffer = dev.createBuffer(bufferSize,
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                        vertexBufferMemory);

        // Copy from staging to device
        dev.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        // Cleanup staging
        vkDestroyBuffer(dev.getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(dev.getDevice(), stagingMemory, nullptr);
    }

    // ------------------------------------------------------------------------
    // Strip building convenience
    // ------------------------------------------------------------------------

    void LineSet::beginStrip() {
        currentStripPoints.clear();
        currentStripColors.clear();
    }

    void LineSet::addPoint(const glm::vec3& pos, const glm::vec3& color) {
        currentStripPoints.push_back(pos);
        currentStripColors.push_back(color);
    }

    void LineSet::endStrip() {
        if (currentStripPoints.size() < 2) return;

        auto& lineVertices = startWrite();
        for (size_t i = 0; i < currentStripPoints.size(); ++i) {
            lineVertices.push_back({ currentStripPoints[i], currentStripColors[i] });
            // For line list: need to duplicate interior vertices to create connected strips
            if (i > 0 && i < currentStripPoints.size() - 1) {
                lineVertices.push_back({ currentStripPoints[i], currentStripColors[i] });
            }
        }
        endWrite();
    }

}