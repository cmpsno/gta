# ROADMAP.md: Palm District → Tilted Towers

**Goal:** Turn the flat Palm District prototype into a dense, vertical, Tilted Towers-style environment: multi-floor interiors, rooftops, tight urban collision.
**Conventions and decisions:** see `CLAUDE.md`. Delete or archive this file when Phase 3 ships.

## Where we are (2026-10-02)

**Phase 1 is complete.** All 9 tasks merged into `v0.4`, 7 test suites green.

What was built:
- `Vec3` + `AABB` foundation; `Player` and `Vehicle` on `Vec3`.
- Floor system: `World::floors` slabs, `findFloorY` gravity, `findStepTop` step-up (0.6 m).
- 3D collision: movers are vertical cylinders (circle on XZ, rotation-invariant for yawing bodies), tested against `Block{AABB bounds, palette}` footprints gated on Y overlap. Axis-separated X/Z stepping; Y resolves via floors/gravity. No swept collision needed — no tunneling observed.
- One two-floor building at (-30, 29): hollow shell, ground-floor walls, 6-step interior staircase, second-floor slab, second-floor walls, walk-through east doorway. Upper walls block only the floor they're on (that's what the Y-gating was for).
- Test staircase, doorway entry, and a dedicated `sandbox_collision` CTest target.

Deviations from the original plan worth knowing:
- Collision is cylinder-vs-block, not pure AABB-vs-AABB (circles are correct for yawing bodies; AABB was too conservative at corners).
- The two-floor building lives in the exterior coordinate space — entry is a physical doorway, not a teleport. `Mode::Interior` still means "the studio's separate space" for now.
- Pushing via the git-database API uploads full files, not diffs — stacked local commits can leak one task's content into another task's push. Fixed once (PR #18); CI (Phase 2 task 0) prevents recurrence.

**Broken window:** commits `d41b4ea` through `b324a8c` do not build (the task-7 push delivered `simulation.cpp` without its matching header). Fixed by PR #18. If bisecting, `git bisect skip` that range.

## Phase 2: Urban authoring (active)

**Goal:** A dense, multi-story environment defined in data.

**Done when:**
- At least 3 enterable buildings, each with 3+ floors.
- Player can enter, climb to the roof, and look down at the street.
- Entering and leaving a building switches interior mode and minimap state with no teleport or hitch.
- Buildings can be edited without recompiling.

Tasks (one small PR each):

0. **Add CI.** Headless build + `ctest`, syntax-only compile of the graphical sources, required checks on `v0.4`. ~20 lines of workflow. If the API push is still in use, add a pre-push local build check to the push script. This pays off immediately — Phase 2 means many more pushes.
1. **Refresh `CLAUDE.md`.** It's stale: still documents `Box` as `x, z, width, depth, height` and the ground as implicit at y = 0. Update it to `Block{AABB, palette}`, the `floors` slab system, and cylinder-vs-block collision.
2. **Data-driven buildings.** Extract the hardcoded `World` constructor into a struct array in code (buildings, walls, floors, stairs, doorways as data). The two-floor prototype at (-30, 29) becomes the first data-defined building. Acceptance: a test compares the data-built world against a snapshot of the old one — same block count, same bounds, same floors. That turns "no behavior change" from a claim into a check.
3. **Second enterable building.** Convert one more solid block into a hollow 2-floor shell via the data format (walls + slab + stairs + doorway). Proves the format generalizes.
4. **Third enterable building, 3 floors.** A 3-story building with two staircases. Proves vertical stacking beyond 2 floors.
5. **Interior instancing.** Generalize the interior system: `Mode::Interior` becomes "inside building X" instead of "the studio". The studio becomes interior 0; the data-driven buildings become interiors 1+. Entering is still walk-through (same space); the mode drives camera distance and minimap. Note: this may force changes to the data format from tasks 2–4 — keep the format cheap to change until this lands.
6. **Per-floor detail.** Furniture and railing AABBs on each floor of the enterable buildings (colliders + rendered boxes). Stairs get railings.
7. **Rooftop access.** Extend one building with a stairwell to a walkable roof; roof edges get railing walls. Player can stand on the roof and look down.
8. **Minimap shows the current floor.** When inside a building, the minimap indicates the player's floor (e.g., "F2") and draws that floor's walls.
9. **Edit without recompiling.** Move the building struct array into a plain-text data file loaded at startup (simple format first; JSON only if it earns its keep). Decide upfront: (a) how the file path is resolved — the working directory differs between the game and `ctest`, so use a compile-time define or CMake-set variable; (b) what the loader does on a malformed file — a clear error naming the line beats a silent fallback, since "edit and restart" will hit malformed files quickly. Verify: edit the file, restart, see the change — no rebuild.

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

## Risks

| Risk | Impact | Mitigation |
|---|---|---|
| 3D collision complexity | High | Done — cylinder-vs-block with Y-gating; swept only if tunneling appears |
| Scope creep | High | Finish Phase 2 acceptance before anything else |
| Stair jitter | Medium | Step-up tolerance plus snap-to-floor when grounded |
| Many AABBs hurt performance | Medium | Phase 3: spatial hash broad-phase; frustum culling |
| Studio interior load hitches | Low | Preload; only applies to the studio's separate space |
| Data format over-engineering | Medium | Struct array first, JSON later |
| API-push content leaks | Medium | CI build+test on every PR (task 0), plus pre-push local build check |

## Sketches (illustrative only, not final code)

```cpp
// AABB overlap
bool overlaps(const AABB& a, const AABB& b) {
    return a.min.x < b.max.x && a.max.x > b.min.x &&
           a.min.y < b.max.y && a.max.y > b.min.y &&
           a.min.z < b.max.z && a.max.z > b.min.z;
}

// Highest floor top at or below the player, over the player's XZ position
float findFloorY(const Vec3& pos, const std::vector<AABB>& floors) {
    float best = -INFINITY;
    for (const auto& f : floors) {
        bool inside = pos.x >= f.min.x && pos.x <= f.max.x &&
                      pos.z >= f.min.z && pos.z <= f.max.z;
        if (inside && f.max.y <= pos.y && f.max.y > best) best = f.max.y;
    }
    return best;
}

// Movers are vertical cylinders: XZ circle + Y range, tested against blocks
// only when the Y ranges overlap (upper-floor walls don't block the ground).
```

## Handoff checklist

- [x] Phase 1 complete: 9/9 tasks merged, 7/7 test suites green.
- [ ] CI in place (task 0).
- [ ] `CLAUDE.md` refreshed (task 1).
- [ ] Phase 2 tasks filed as GitHub issues.
- [ ] Data-driven building format agreed (struct array first).
