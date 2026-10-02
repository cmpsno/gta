#include "simulation.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace palm;
int assertions = 0;
void check(bool ok, const std::string& message) {
    ++assertions;
    if (!ok) throw std::runtime_error(message);
}
void near(float a, float b, float tolerance, const char* message) {
    check(std::abs(a - b) < tolerance, message);
}

int main() {
    try {
        // Phase 1 task 9: AABB overlap primitives (3D).
        {
            AABB box{{-1,-1,-1},{1,1,1}};
            AABB inner{{-0.5f,-0.5f,-0.5f},{0.5f,0.5f,0.5f}};
            AABB far{{5,5,5},{6,6,6}};
            AABB touch{{1,1,1},{2,2,2}};
            check(overlaps(box, inner), "Contained AABBs overlap");
            check(!overlaps(box, far), "Separated AABBs do not overlap");
            check(!overlaps(box, touch), "Touching faces do not overlap");
            // Slab vs standing volume: the step-up's world.
            AABB slab{{-2,0,-2},{2,0.5f,2}};
            AABB standing{{-0.5f,0,-0.5f},{0.5f,1.8f,0.5f}};
            AABB above{{-0.5f,1,-0.5f},{0.5f,2.8f,0.5f}};
            check(overlaps(slab, standing), "Floor slab overlaps standing volume");
            check(!overlaps(slab, above), "Volume above slab does not overlap");
        }

        // Phase 1 task 9: freePosition Y-gating. A mover is a vertical
        // cylinder; a block collides only if the XZ circle hits AND the Y
        // ranges overlap.
        {
            Simulation s;
            s.world.buildings.clear();
            s.world.buildings.push_back({{{6,3,-4},{10,5,4}}, 0}); // floating wall
            // Walker below the wall (body y 0..1.8): no Y overlap, passes.
            check(s.freePosition(playerAABB({8,0,-10}), false),
                  "Floating wall does not block the ground floor");
            // Walker at the wall's height (body y 3..4.8): Y overlaps, blocked.
            check(!s.freePosition(playerAABB({8,3,0}), false),
                  "Floating wall blocks at its own height");
            // Grounded wall blocks the ground floor.
            s.world.buildings.clear();
            s.world.buildings.push_back({{{6,0,-4},{10,5,4}}, 0});
            check(!s.freePosition(playerAABB({8,0,0}), false),
                  "Grounded wall blocks the walker");
            // But not a flyer above it (body y 6..7.8).
            check(s.freePosition(playerAABB({8,6,0}), false),
                  "Grounded wall does not block above its top");
        }

        // Phase 1 task 9: findStepTop direct unit tests.
        {
            std::vector<AABB> fl = {{{-104,-1,-104},{104,0,104}}};
            fl.push_back({{-2,0,-2},{2,0.5f,2}});
            fl.push_back({{-2,0,2},{2,1.0f,4}});
            near(findStepTop({0,0,0}, fl, 0.6f), 0.5f, 1e-6f, "Step within reach snaps up");
            near(findStepTop({0,0,3}, fl, 0.6f), 0, 1e-6f, "Step above maxStep ignored");
            near(findStepTop({10,0,10}, fl, 0.6f), 0, 1e-6f, "Outside step XZ, no snap");
            fl.push_back({{-2,0,-2},{2,0.3f,2}});
            near(findStepTop({0,0,0}, fl, 0.6f), 0.3f, 1e-6f, "Lowest reachable step wins");
        }

        std::cout << "OK collision (" << assertions << " assertions)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "FAIL: " << e.what() << "\n";
        return 1;
    }
}
