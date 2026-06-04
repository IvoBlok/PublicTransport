#pragma once

#include <renderer/coreTypes.hpp>
#include <renderer/objectTypes/baseType.hpp>

#include <vector>

// TODO: write this into a LineSet class instead, such that it can store multiple lines within a single buffer (saving atomics and draw calls). If done properly, I could probably still keep it usable if I have a set of 1 line total
namespace renderer {
    // define Line class, which contains a series of LineVertices and potentially other information (transparency, on/off in the renderer, etc...)
    // it should handle the creation of the vulkan buffers to hold the vertices for now. If at some point we are limited by GPU memory speed, or the number of draw calls, 
    class Line {
    public:
        Line(VulkanContext& renderContext);
        ~Line();
        
		Line(const Line&) = delete;
		Line& operator=(const Line&) = delete;

		Line(Line&& other) noexcept;
		Line& operator=(Line&& other) noexcept;

        bool needsUpdate() const;
        void updateGPU();

        void render(VkCommandBuffer commandBuffer) const;

        auto& startWrite() { return vertices.startWrite(); }
        void endWrite() { vertices.endWrite(); }

    private:
        void createRenderBuffers(const std::vector<RendererVertex>& vertices);
        void destroy();

        VulkanContext& renderContext;

        DoubleBuffer<std::vector<RendererVertex>> vertices;

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        size_t vertexCount;
    };
}