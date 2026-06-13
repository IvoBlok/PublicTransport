# Public Transport Simulator

The idea is to try and reproduce the dutch rail network, simulate a given train schedule, and potentially expand it at some point to include stuff like: 
 - Other forms of public transport
 - Planning the fastest/best/easiest route from A to B
 - Add some form of 'user' dynamics, such that we might be able to measure stuff like how many people use which lines, what changes might be worth trying in the schedule, etc...

# Running
Build and run the main executable, with the interactive renderer/gui:
```bash
meson setup build
meson compile -C build
./build/PTSimulation
```

# Profiling

## Tracy
Firstly, if you want to see both GPU and CPU usage over time, you can profile with tracy. The Tracy Server can be built by running: 
```bash
cd subprojects/tracy
cmake -B profiler/build -S profiler -DCMAKE_BUILD_TYPE=Release -DLEGACY=ON
cmake --build profiler/build --config Release --parallel
```

The client, which also contains our actual Renderer, can be built for profiling with:
```bash
meson setup build -Dtracy_enable=true -Dtracy:on_demand=true
meson compile -C build
```

Then start both the server and client, and select the running client from the server for live profiling:
```bash
./subprojects/tracy/profiler/build/tracy-profiler
sudo ./build/PTSimulation
```

## Perf / Hotspot
For a CPU focussed profiler, with detailed callstacks, you can use perf, with hotspot to visualize the data:
```bash
perf record --call-graph fp ./build/PTSimulation
perf record --call-graph dwarf ./build/PTSimulation
sudo hotspot
```

## Valgrind / KCacheGrind
Alternatively, for CPU focussed profiling we can use valgrind. This gives very similar info to perf / hotspot, but visualizes it differently:
```bash
valgrind --tool=callgrind ./build/PTSimulation
kcachegrind callgrind.out
```

