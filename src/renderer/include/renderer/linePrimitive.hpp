#pragma once

#include "coreTypes.hpp"

#include <vector>
#include <array>

namespace renderer {
    struct LineVertex {
        glm::vec3 pos;
        glm::vec3 color;

        // define binding and attribute descriptions
    };

    // define Line class, which contains a series of LineVertices and potentially other information (transparency, on/off in the renderer, etc...)
    // it should handle the creation of the vulkan buffers to hold the vertices for now. If at some point we are limited by GPU memory speed, or the number of draw calls, 
    class Line {
    public:
        Line(VulkanContext& context);
        std::vector<LineVertex> vertices;
    
    private:
        VulkanContext& context;
    };
}