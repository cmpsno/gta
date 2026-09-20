// V0.4 headless stress benchmark: measures Simulation::step() cost as the
// NPC count scales. Prints a table; always exits 0 (measurement, not a gate).
// Concrete numbers go into docs/V0.4_PROFILING.md.
#include "simulation.h"
#include <chrono>
#include <cstdio>

using namespace palm;
int main() {
    std::printf("%-6s %-12s %-12s %-16s\n","npcs","total_ms","us_per_step","checks_per_step");
    for (int n : {1,10,25,50,100}) {
        Simulation s;
        s.setStressNpcs(n);
        Input walk; walk.movement={0,1}; walk.sprint=true;
        const int steps=1200; // 10 simulated seconds at 120 Hz
        long checks=0;
        auto t0=std::chrono::steady_clock::now();
        for (int i=0;i<steps;++i) { s.step(walk,FixedStep); checks+=s.collisionChecks; }
        auto t1=std::chrono::steady_clock::now();
        double ms=std::chrono::duration<double,std::milli>(t1-t0).count();
        std::printf("%-6d %-12.2f %-12.2f %-16.1f\n",
                    n,ms,ms*1000.0/steps,static_cast<double>(checks)/steps);
    }
    return 0;
}
