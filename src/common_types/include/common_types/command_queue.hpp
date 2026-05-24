#pragma once

// idea is that the compute code owns a queue of commands, which the renderer can add to via a GUI or smth. 
// this is hopefully also somewhat chosen such that independent parallelism between compute and rendering is more straight-forward
enum class ComputeCommand {
    START_SIMULATION,
    PAUSE_SIMULATION,
    RESET
};