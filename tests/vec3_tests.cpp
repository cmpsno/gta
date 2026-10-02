#include "vec3.h"
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
        Vec3 a{1, 2, 3}, b{4, -1, 0.5f};
        Vec3 sum = a + b;
        near(sum.x, 5, 1e-6f, "Vec3 add x");
        near(sum.y, 1, 1e-6f, "Vec3 add y");
        near(sum.z, 3.5f, 1e-6f, "Vec3 add z");
        Vec3 diff = a - b;
        near(diff.x, -3, 1e-6f, "Vec3 sub x");
        Vec3 scaled = a * 2;
        near(scaled.y, 4, 1e-6f, "Vec3 scalar mul y");
        near(length(Vec3{3, 4, 0}), 5, 1e-6f, "Vec3 length");
        Vec3 n = normalized(Vec3{0, 0, 5});
        near(n.z, 1, 1e-6f, "Vec3 normalized");
        near(length(normalized(Vec3{0, 0, 0})), 0, 1e-6f, "Vec3 normalized zero");

        AABB box = aabbFromCenter({0, 0, 0}, {1, 1, 1});
        near(box.min.x, -1, 1e-6f, "aabbFromCenter min");
        near(box.max.y, 1, 1e-6f, "aabbFromCenter max");
        AABB inner = aabbFromCenter({0, 0, 0}, {0.5f, 0.5f, 0.5f});
        check(overlaps(box, inner), "contained AABBs overlap");
        AABB far = aabbFromCenter({10, 0, 0}, {1, 1, 1});
        check(!overlaps(box, far), "separated AABBs do not overlap");
        AABB touch = aabbFromCenter({2, 0, 0}, {1, 1, 1});
        check(!overlaps(box, touch), "touching faces do not overlap");
        AABB slab = {{-5, 0, -5}, {5, 0.2f, 5}};
        AABB tall = {{-1, 0.1f, -1}, {1, 3, 1}};
        check(overlaps(slab, tall), "floor slab overlaps standing volume");
        AABB above = {{-1, 3.1f, -1}, {1, 5, 1}};
        check(!overlaps(slab, above), "volume above slab does not overlap");
    } catch (const std::exception& e) {
        std::cout << "FAIL: " << e.what() << "\n";
        return 1;
    }
    std::cout << "OK vec3 (" << assertions << " assertions)\n";
    return 0;
}
