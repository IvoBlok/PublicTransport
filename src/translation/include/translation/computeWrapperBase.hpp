#pragma once

#include <renderer/renderables/renderable.hpp>

#include <vector>


/*
ComputeWrapperBase is the base class from which all translation classes are derived. The translation classes are, if created, responsible for:
 - holding its simulation / compute class (ex. PathFindingOnGraph)
 - defining the GUI to interact with and define the parameters / inputs to the respective compute / simulation
 - defining the callbacks for the compute algorithm to refer to when returning given results (be it intermediate, or final results)
 - defining the behaviour of how the results are interpreted and used; are numerical outputs plotted, added to the gui, are parts of the output rendered as arrows, boxes, cubes, splines?
*/
class ComputeWrapperBase {
public:
    virtual ~ComputeWrapperBase() = default;

    virtual void renderGUI() = 0;

    virtual renderer::Renderable* getRenderable() = 0;
};