#pragma once

#include <vector>
#include <functional>

namespace compute {
    class ExampleSimulation {
    public:
        struct Parameters {
            float viscosity = 0.01f;
            float density = 1000.0f;
            float timestep = 0.016f;
            float gridSize = 64;
        };

        struct State {
            std::vector<float> velocity;
            std::vector<float> pressure;
            float totalEnergy = 0.0f;
            int iteration = 0;
        };

        using StateCallback = std::function<void(const State&)>;

        const Parameters& getParameters() const { return params; }
        void setParameters(Parameters& inputParams) { params = inputParams; }


        struct RunParameters {
            int numSteps = 0;
        };
        void run(RunParameters inputs, StateCallback callback) {
            for (int i = 0; i < inputs.numSteps; i++)
                state.velocity.push_back(1.0f);

            callback(state);
        }

    private:
        Parameters params;
        State state;
    };

}