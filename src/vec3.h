#pragma once

// Phase 1 handoff: 3D math primitives. Added ahead of the migration so the
// rest of Phase 1 can adopt them one struct at a time. Nothing in the
// simulation uses these yet; behavior is unchanged.
namespace palm {

struct Vec3 {
    float x = 0, y = 0, z = 0;
};
Vec3 operator+(Vec3 a, Vec3 b);
Vec3 operator-(Vec3 a, Vec3 b);
Vec3 operator*(Vec3 a, float s);
float length(Vec3 v);
Vec3 normalized(Vec3 v);

// Axis-aligned bounding box, the single 3D collider for Phase 1.
// Convention: min <= max component-wise. Volumes are closed on min,
// open on max, matching the existing XZ-plane `overlaps` semantics
// (touching edges do not count as overlapping).
struct AABB {
    Vec3 min{};
    Vec3 max{};
};
bool overlaps(const AABB& a, const AABB& b);

// Build an AABB from center + half-extents, the common authoring form.
inline AABB aabbFromCenter(Vec3 center, Vec3 halfExtents) {
    return AABB{center - halfExtents, center + halfExtents};
}

} // namespace palm
