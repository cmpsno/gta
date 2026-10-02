#include "vec3.h"
#include <cmath>

namespace palm {

Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
float length(Vec3 v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
Vec3 normalized(Vec3 v) {
    float n = length(v);
    return n > 0.0001f ? v * (1 / n) : Vec3{};
}
bool overlaps(const AABB& a, const AABB& b) {
    return a.min.x < b.max.x && a.max.x > b.min.x && //
           a.min.y < b.max.y && a.max.y > b.min.y && //
           a.min.z < b.max.z && a.max.z > b.min.z;
}

} // namespace palm
