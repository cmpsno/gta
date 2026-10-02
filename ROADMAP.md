# ROADMAP.md: Palm District → Tilted Towers

**Handoff to:** Muse
**Goal:** Turn the flat Palm District prototype into a dense, vertical, Tilted Towers-style environment: multi-floor interiors, rooftops, tight urban collision.
**Conventions and decisions:** see `CLAUDE.md`. Delete or archive this file when Phase 3 ships.

## Why this order

The prototype proves walking, driving, entering a building, and swimming, but the world is a flat plane with a height variable. Moving to true 3D position and collision is the critical path. Building authoring, interiors, and performance all depend on it.

## Phase 1: 3D core (critical path)

**Goal:** Move vertically, stand on floors, climb stairs, collide with 3D AABBs.

1. Introduce `Vec3`; migrate `Player`, `Vehicle`, `World` positions.
2. Replace `Box` with `AABB { Vector3 min, max; }`.
3. Add floor system: horizontal slabs at Y levels; gravity pulls to the nearest floor below.
4. Stairs/ramps: start with thin AABB steps plus a step-up tolerance; move to `Ramp` volumes if that jitters.
5. Update collision to 3D, resolving X, Y, Z separately. Add swept collision only if tunneling appears.
6. Update the camera to follow in 3D with vertical smoothing.

**Done when:**
- Player walks up a ramp to a second floor without falling through.
- Player jumps onto a raised platform.
- Walls block the player on every floor.
- Vehicle collides correctly with 3D building volumes.
- All V0 features (driving, swimming, car entry) still work, with no build or control regressions.

## Phase 2: Urban authoring

**Goal:** A dense, multi-story environment defined in data.

1. Data-driven buildings: replace the hardcoded `World` constructor with a loader. Start with a struct array in code; move to JSON only when it earns its keep.
2. Interior instancing: `Mode::Interior` becomes "inside building X"; support multiple enterable buildings.
3. Multi-floor interiors with per-floor wall, furniture, and railing AABBs; stairs connect floors.
4. Rooftop access via stairwells, elevators, or fire escapes; rooftops have edges and railings.
5. Minimap shows the current floor.

**Done when:**
- At least 3 enterable buildings, each with 3+ floors.
- Player can enter, climb to the roof, and look down at the street.
- Interiors load and unload without restarting.
- Buildings can be edited without recompiling.

## Phase 3: Performance and feel

**Goal:** 60 FPS in a dense vertical area; movement feels good.

1. Frustum culling for all AABBs; spatial hash or grid for broad-phase collision.
2. Portal/occlusion culling for interiors and building clusters.
3. Simple-box LOD for distant buildings.
4. Controller polish: crouching, no stair jitter (step-up tolerance plus snap-to-floor when grounded), optional vaulting.
5. Camera collision so it never clips through walls.

**Done when:**
- 60 FPS at 1080p on mid-range hardware with 20+ buildings.
- No camera clipping in tight interiors.
- Smooth stair traversal.

## Later (optional, post-foundation)

Non-combat content only: ambient NPCs with patrol routes that can use stairs, traversal missions (rooftop objectives, deliveries, chases), day/night cycle, vehicle traffic. Combat is out of scope.

## First tasks (one small PR each)

1. Add `Vec3` and `AABB` headers; no behavior change.
2. Migrate `Player` to `Vec3` (`height` becomes `position.y`).
3. Migrate `Vehicle` to `Vec3`; keep it on the ground.
4. Add floor detection; player still stands at y = 0.
5. Add one test ramp and verify the player can walk up it.
6. Replace `Box` with `AABB`; make collision resolution 3D.
7. Add a second floor to one building (slab, stairs, walls).
8. Refactor the interior transition to support entering that building.
9. Write unit tests for AABB collision and floor detection.

Turn these into GitHub issues tagged `handoff`.

## Risks

| Risk | Impact | Mitigation |
|---|---|---|
| 3D collision complexity | High | AABB with per-axis resolution first; swept only if needed |
| Scope creep | High | Finish Phase 1 acceptance before anything else |
| Stair jitter | Medium | Step-up tolerance plus snap-to-floor when grounded |
| Many AABBs hurt performance | Medium | Early frustum culling; spatial hash broad-phase |
| Interior load hitches | Medium | Preload adjacent interiors (small scope keeps this cheap) |
| Data format over-engineering | Medium | Struct array first, JSON later |

## Sketches (illustrative only, not final code)

```cpp
// AABB overlap
bool overlaps(const AABB& a, const AABB& b) {
    return a.min.x < b.max.x && a.max.x > b.min.x &&
           a.min.y < b.max.y && a.max.y > b.min.y &&
           a.min.z < b.max.z && a.max.z > b.min.z;
}

// Highest floor top at or below the player, over the player's XZ position
float findFloorY(const Vector3& pos, const std::vector<AABB>& floors) {
    float best = -INFINITY;
    for (const auto& f : floors) {
        bool inside = pos.x >= f.min.x && pos.x <= f.max.x &&
                      pos.z >= f.min.z && pos.z <= f.max.z;
        if (inside && f.max.y <= pos.y && f.max.y > best) best = f.max.y;
    }
    return best;
}

// Step-up: after the horizontal move, if grounded and blocked by a step
// no taller than maxStepHeight, raise Y to the step top.
```

## Handoff checklist

- [ ] Muse has read `CLAUDE.md` and this file.
- [ ] Build verified on Muse's machine.
- [ ] Claims in `CLAUDE.md` verified against the live repo.
- [ ] Phase 1 tasks filed as GitHub issues.
- [ ] First PR (`Vec3` + `AABB`) merged.
