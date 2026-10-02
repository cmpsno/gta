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
void run(Simulation& s, const Input& input, int steps) {
    for (int i = 0; i < steps; ++i) s.step(input, FixedStep);
}
int main() {
    try {
        Simulation s;
        // The ground slab is always there: standing anywhere, the floor is 0.
        near(findFloorY({0, 0, 0}, s.world.floors), 0, 1e-6f, "Ground slab under spawn");
        near(findFloorY({-100, 0, 100}, s.world.floors), 0, 1e-6f, "Ground slab at world edge");
        near(findFloorY({0, 3, 0}, s.world.floors), 0, 1e-6f, "Ground slab below airborne player");

        // A raised platform: the query must pick the highest slab at/below the player.
        std::vector<AABB> floors = s.world.floors;
        floors.push_back({{-5, 2, -5}, {5, 2.2f, 5}}); // platform, top at 2.2
        near(findFloorY({0, 1, 0}, floors), 0, 1e-6f, "Slab above the player is not the floor");
        near(findFloorY({0, 5, 0}, floors), 2.2f, 1e-6f, "Raised slab is the floor when below the player");
        near(findFloorY({0, 2.2f, 0}, floors), 2.2f, 1e-6f, "Standing exactly on the slab");
        near(findFloorY({20, 5, 0}, floors), 0, 1e-6f, "Outside the slab XZ, floor is the ground");

        // Stacked slabs: the highest one at/below the player wins.
        floors.push_back({{-5, 4, -5}, {5, 4.3f, 5}});
        near(findFloorY({0, 5, 0}, floors), 4.3f, 1e-6f, "Highest slab below the player wins");
        near(findFloorY({0, 3, 0}, floors), 2.2f, 1e-6f, "Lower slab when the top one is above");

        // Gameplay: with only the ground slab, the player still stands at y = 0.
        s.step({}, FixedStep);
        near(s.player.position.y, 0, 1e-6f, "Player rests on the ground slab");
        Input jump;
        jump.jump = true;
        s.step(jump, FixedStep);
        check(s.player.position.y > 0, "Jump leaves the floor");
        for (int i = 0; i < 120; ++i) s.step({}, FixedStep);
        near(s.player.position.y, 0, 1e-6f, "Jump lands back on the floor");
    } catch (const std::exception& e) {
        std::cout << "FAIL: " << e.what() << "\n";
        return 1;
    }
    std::cout << "OK floor (" << assertions << " assertions)\n";
    return 0;
}
