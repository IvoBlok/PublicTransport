#pragma once

#include <renderer/coreTypes.hpp>

#include <vector>
#include <array>

namespace renderer {
    // define Line class, which contains a series of LineVertices and potentially other information (transparency, on/off in the renderer, etc...)
    // it should handle the creation of the vulkan buffers to hold the vertices for now. If at some point we are limited by GPU memory speed, or the number of draw calls, 
    class Line {
    public:
        Line(VulkanContext& renderContext);
        ~Line();
        
        void render(VkCommandBuffer commandBuffer);
        void createVertexBuffer();
        void destroy();

    private:
        std::vector<RendererVertex> vertices; // temp type, it should probably get its own type
        VulkanContext& renderContext;

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
    };
}