# CLAUDE.md: Project Conventions

C++17 / raylib 3D sandbox (`cmpsno/gta`). Currently the "Palm District" prototype; evolving into a Tilted Towers-style vertical urban environment (see `ROADMAP.md`).

> **Verify before trusting.** Names, paths, and struct layouts below were written from a previous review of the repo. Check them against the live code and fix any that are wrong.

## Project decisions

- **Platform:** PC only.
- **Players:** single-player only. No networking.
- **Art:** blocky primitives (boxes). No textured assets yet.
- **Scope:** one POI plus a small surrounding area. Fits in memory; no world streaming.
- **Combat:** out of scope. This is a traversal sandbox.

## Build and run

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
./gta
```

Requirements: C++17 compiler, CMake >= 3.15, raylib (version pinned in CMake). Optional: Catch2 or doctest for tests.

## Controls (verify)

| Key | Action |
|---|---|
| WASD | Move |
| Mouse | Look |
| Shift | Sprint |
| Space | Jump / handbrake |
| E | Enter/exit vehicle |
| Esc | Pause/exit |

## Architecture

- **Fixed timestep:** `1.0 / 120.0` s with an accumulator. `Simulation::step(dt)` advances state.
- **Simulation and rendering are separate.** `Simulation` owns `World`, `Player`, `Vehicle`, and mode state.
- **Player modes:** `OnFoot`, `Driving`, `Interior`. Swimming is a `Player.swimming` flag, not a mode.
- **World:** primitive-driven; buildings and colliders are axis-aligned boxes defined in code.
- **Rendering:** raylib `DrawCube` / `DrawCubeWires`, third-person follow camera, distance culling, top-down minimap.
- **Input:** centralized mapping; raw key states polled each frame.

### Current state (being migrated)

- `Player.position` is `Vec3` (`height` is now `position.y`). `Vehicle.position`
  is `Vec3` too, pinned at y = 0. NPC positions are still `Vec2 (x, z)`;
  `planar()` projects to XZ until task 6.
- Floors are explicit AABB slabs (`World::floors`, ground slab included);
  gravity pulls the player to `findFloorY`, not to a hardcoded y = 0.
- Step-up: after the horizontal move, a slab top within `MaxStepHeight`
  (0.6 m) above the feet snaps the player up. Test staircase in the world
  (x in [-14,-10], z in [20,38], five 0.5 m steps to a 2.5 m platform).
- `Box` is `x, z, width, depth, height`; collision resolves on the XZ plane.
- Ground is implicit at y = 0.
- One hardcoded interior room.

### Target state

| Concept | Target |
|---|---|
| Position | `Vec3` |
| Collider | `AABB` (min/max in X/Y/Z) |
| Floors | Explicit floor slabs at multiple Y levels |
| Buildings | Multi-floor, with interiors, stairs, doors |
| Interiors | Instanced per building |
| Culling | Frustum, then portal/occlusion for tall buildings |

## Rules

- **Simulation never calls raylib rendering functions.**
- **Simulation stays deterministic.** No `rand()` without seed control.
- No raw owning pointers; use `std::unique_ptr` / `std::vector`.
- Don't add features outside the current roadmap phase.

## Style

- Format with clang-format (use the repo's `.clang-format` if present).
- Types `PascalCase`, functions `camelCase`, members `camelCase` (`m_` prefix only if the repo already uses it).
- Match the existing header-guard style and namespace.
- Small, mergeable PRs; one concern each.

## Testing

- **Unit (Catch2/doctest):** AABB overlap, ray-AABB, floor detection, stair step-up.
- **Integration:** scripted-input `Simulation::step(dt)` runs. Examples: player walks up a ramp to the second floor; vehicle hits a building and stops.
- **Manual checklist:** enter/exit car, swim, enter building and climb to roof, jump off roof to street, drive into building with no clipping.
