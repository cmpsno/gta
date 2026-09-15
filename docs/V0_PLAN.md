# V0 plan — Palm District

## Goal

Build an environment-first, GTA-inspired learning sandbox in C++. A complete five-minute loop is: **walk → drive → stop and exit → enter a room → return outside → swim → return to shore**.

The handwritten annotations are design input. The printed worksheet questions are reference prompts, not instructions to execute literally. The previous repository research/modding proposal is preserved in `original-research-notes.md`; it does not define this game implementation.

## Scope from the notes

| Your intention | V0 decision | Reason |
|---|---|---|
| Environment first; primitives are enough | A small 3D district with solid buildings, roads, sidewalks, and a beach | Tests interactions without asset production |
| Walk, run, jump; maybe swimming | Camera-relative movement, sprint, jump arc, surface swimming | Makes position, velocity, and locomotion states visible |
| Cannot run through buildings | Player/car collision and world bounds | Makes space constrain movement |
| Drive; transfer control from person to vehicle | One car with acceleration, reverse, steering, coasting, braking, safe exit | Proves the interaction before traffic or AI |
| Enter a building through proximity/input/state change | One empty studio, its own room, and a return point | Teaches transitions while shopping and animations wait |
| NPCs, police, weapons are premature | Defer them | They depend on stable movement and interaction ownership |
| Missions and money are uncertain/deferred | Omit them; optional mechanic discovery indicators only | The sandbox can be evaluated without an economy or objectives |

**Working assumption:** third-person 3D with primitive shapes. The notes' rectangles and circles become cuboids and simple circular collision footprints. One small room resolves the notes' tension between deferring stores and exploring building entry: it demonstrates entry without shopping.

## Build order and acceptance criteria

### 1. Locomotion and a readable world

Create the window and neighborhood; store position, facing, height/vertical velocity on the player. Translate WASD relative to the camera, normalize diagonal movement, and update simulation at 120 Hz separately from rendering. Include sprint, jump, camera orbit/zoom, pause, and reset.

**Accept when:** one second of walking covers 4.5 m, sprint covers 9 m, diagonals are no faster, and jumps return to ground. Losing focus pauses the game.

### 2. Collision makes space meaningful

Share building boxes between collision and rendering. Move in short steps, resolving horizontal axes independently. Bound the district and shorten the camera against walls.

**Accept when:** running into a wall stops penetration, tangential input slides along it, and the player/car cannot escape the map.

### 3. One car, two control modes

The vehicle owns position, heading, speed, and steering. Require a grounded, nearby player before entry. Switch input ownership and hide the standing player while occupied. Support forward/reverse, steering, coasting, and braking. Exit requires low speed and a free landing position; reject it if every candidate is blocked.

**Accept when:** approach → enter → drive → stop → exit works without controlling both objects or spawning inside solid geometry.

### 4. One interior transition

Define an exterior entrance and a room exit. A nearby E press changes the scene, sets a safe spawn, and switches to room collision geometry. Returning outside restores a safe street position.

**Accept when:** repeated entry/exit works, distant E presses do nothing, and room walls/furniture constrain movement.

### 5. Water and delivery

Crossing the shoreline at ground level enables slower swimming. Returning to shore restores walking. Cars stop before the water. Add controls, map, interaction prompts, and optional discovery indicators. Package a Windows executable, document the architecture, test the mechanics, and inspect rendered scenes.

**Accept when:** the complete five-minute loop works, core checks pass, and street/car/room/water scenes render correctly.

## Object ownership

| Object | Owns | Relationships |
|---|---|---|
| Player | Position, facing, jump height/velocity, swimming flag | Collides, enters car/room, crosses shore |
| Vehicle | Position, heading, speed, steering | Receives input while occupied, collides, offers safe exits |
| World | Building/room boxes, entrance/exit positions, bounds | Supplies solid geometry and transition locations |
| Simulation | Control mode, discoveries, temporary prompts | Routes input and coordinates interactions |
| View | Camera, font, help/pause/debug display | Draws simulation state |

An eventual NPC should own position, velocity, destination, and behavior state so multiple NPCs can act independently. Visibility optimization is a separate rendering concern; independent state is necessary even when all NPCs are visible.

## State transitions

```text
On foot -- near car + E + grounded --> Driving
Driving -- stopped + E + safe exit --> On foot
On foot -- studio door + E + grounded --> Interior
Interior -- exit door + E --> On foot
On foot -- crosses water edge at surface --> Swimming (locomotion flag)
Swimming -- reaches shore --> Walking
Any state -- R --> Fresh outdoor spawn
```

Hidden steps in entering the studio: know both positions → measure distance → detect a single E press → check eligibility → change scene → set safe spawn → reset camera target → use room collision → expose a return interaction.

## Validation strategy

Headless tests cover normalized movement, sprint, jump/landing, wall and boundary collision, car proximity and control, acceleration/reverse/steering/braking, moving/blocked exits, shoreline protection, room transitions, swimming, reset, and invalid time. Checks remain active in release builds.

Graphical smoke runs render 120 frames in an actual OpenGL context and capture the street, occupied car, studio, water, and welcome screen. Scene presets inspect presentation, while core tests verify interaction rules. A human playthrough is still needed to judge keyboard/mouse feel.

## Deliberate simplifications

Finite 208 x 208 m map; one car; one room; circular car collision; planar walls with a separate jump arc; no rooftop climbing; surface swimming without buoyancy; a scene transition rather than seamless interiors. Trees and markings are decorative. No audio, saves, NPCs, traffic, police, weapons, missions, money, stores, dialogue, or multiplayer.

## Next milestones

1. **V0.1 — feel:** tune movement, camera, steering, and feedback based on playtesting.
2. **V0.2 — one NPC:** an independently owned character following waypoints and respecting walls. No crowds yet.
3. **V0.3 — reusable interactions:** generalize proximity targets, then add another car or room.
4. **V0.4 — measured scale:** profile frame time and object count before adding spatial indexing, chunks, or advanced culling.
5. Only then consider traffic, wanted states, combat, and missions as separate features with explicit dependencies.

Avoid a custom allocator, ECS framework, general mission system, or asset streaming until measurements or actual gameplay requirements justify it.
