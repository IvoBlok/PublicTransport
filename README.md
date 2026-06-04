# Public Transport Simulator

The idea is to try and reproduce the dutch rail network, simulate a given train schedule, and potentially expand it at some point to include stuff like: 
 - Other forms of public transport
 - Planning the fastest/best/easiest route from A to B
 - Add some form of 'user' dynamics, such that we might be able to measure stuff like how many people use which lines, what changes might be worth trying in the schedule, etc...

# Profiling

I use tracy here, and installation is managed by meson. The Tracy Server can be built by running: 
```bash
cd subprojects/tracy
cmake -B profiler/build -S profiler -DCMAKE_BUILD_TYPE=Release -DLEGACY=ON
cmake --build profiler/build --config Release --parallel
```

The client, which also contains our actual Renderer, can be built for profiling with:
```bash
meson setup build -Dtracy_enable=true
meson compile -C build
```

Then start both the server and client, and select the running client from the server for live profiling:
```bash
./subprojects/tracy/profiler/build/tracy-profiler
./build/PTSimulation
```