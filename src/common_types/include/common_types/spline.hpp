#pragma once

// The idea is to define datatypes that both renderer and compute here, such that we can keep the interdependence of the 2 to a minimum. 
// Because I also kinda am targeting various types of parallelism, and because we probably want different formats of the same information for the compute and renderer,
// each datatype is stored in 2 different formats. The part (be it compute or rendering) that modifies one of the format is then also responsible for updating the other part (atomically)
// or we can just add a dirty flag (for both directions?) so the actual part that needs it can check it and synchronize when needed. This would also probably keep the separation the cleanest, and keep the costs for compute low if we have an executable with just the compute
