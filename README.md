# Palm District — open-world sandbox v0

<img width="1280" height="800" alt="image" src="https://github.com/user-attachments/assets/ceb7e163-668f-4b94-8661-d7a25bfc9d6e" />


A playable **C++17 / raylib 3D prototype** based on your handwritten GTA-inspired learning notes. Explore a coastal neighborhood, switch between character and vehicle control, enter a studio, and swim along the shore. Everything is made from simple shapes.

## Play on Windows

Extract the accompanying `PalmDistrict-Windows.zip` and double-click `palm_district.exe`. Press **Enter** to start. Requires a desktop graphics driver supporting OpenGL 3.3. The packaged build needs no compiler, account, downloads, or internet connection.

| Input | On foot | In the car |
|---|---|---|
| WASD | Move relative to the camera | W accelerate, S brake/reverse, A/D steer |
| Mouse | Look around | Look around |
| Shift | Sprint | — |
| Space | Jump | Brake |
| E | Enter nearby car or studio | Exit when stopped |
| Wheel | Camera distance | Camera distance |
| H / F3 | Controls / collision display | Controls / collision display |
| R | Reset sandbox | Reset sandbox |
| Escape | Pause and release mouse | Pause and release mouse |
| Enter / Q | Resume / quit when paused | Resume / quit when paused |

Losing window focus pauses the game. Close the window at any time to quit.

### A five-minute first play

1. Walk, sprint, and jump. Run into a building: you should stop or slide along it.
2. Approach the coral car near spawn. Press E, drive around a block, brake to a stop, then press E to exit.
3. Find the mint studio door east of the central road, just north of the starting intersection. It is the mint dot on the map. Press E, explore the room, then use its mint exit door.
4. Head east to the palm-lined beach. Walk into the sea to swim; return toward shore to walk again.
5. Six optional indicators track walking 20 m, jumping, entering the car, driving 80 m, visiting the studio, and swimming. These are discoveries, not missions.

## Build from source

Requires CMake 3.20+, Git, a C++17 compiler, and desktop OpenGL 3.3. The first graphical build downloads pinned raylib 5.5 sources.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

For MinGW on Windows, add `-G Ninja` or `-G "MinGW Makefiles"` to the first command. Visual Studio builds place the executable in `build/Release/`; Ninja/Make builds use `build/`.

On Linux, install the GLFW/X11 build prerequisites (Debian/Ubuntu: `build-essential cmake git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`). macOS needs Xcode command-line tools and CMake. **Windows is verified; Linux and macOS have not been tested here.**

Test the simulation without raylib, a graphics context, or network access:

```sh
cmake -S . -B build-core -DBUILD_GAME=OFF
cmake --build build-core --config Release
ctest --test-dir build-core -C Release --output-on-failure
```

An existing raylib checkout can be used with `-DFETCHCONTENT_SOURCE_DIR_RAYLIB=/absolute/path/to/raylib`.

### Rendering checks

```sh
./build/palm_district --smoke --capture street.png
./build/palm_district --smoke --scene drive --capture drive.png
./build/palm_district --smoke --scene studio --capture studio.png
./build/palm_district --smoke --scene water --capture water.png
```

On Windows, use `build/palm_district.exe` (or `build/Release/palm_district.exe`). These render 120 frames in a hidden window and exit. They still require a graphics context. Scene presets inspect presentation; core tests verify interaction rules.

## Where to learn and make changes

| File | Responsibility |
|---|---|
| [docs/V0_PLAN.md](docs/V0_PLAN.md) | Scope, milestones, acceptance criteria, next steps |
| [src/simulation.h](src/simulation.h) | Player, vehicle, world, input, state definitions |
| [src/simulation.cpp](src/simulation.cpp) | Movement, collisions, water, enter/exit rules |
| [src/main.cpp](src/main.cpp) | Window, inputs, pause, fixed simulation clock |
| [src/renderer.cpp](src/renderer.cpp) | City, camera, minimap, interface |
| [tests/simulation_tests.cpp](tests/simulation_tests.cpp) | Headless behavior checks |
| [docs/original-research-notes.md](docs/original-research-notes.md) | Preserved previous README |

Start with `Simulation::interact()` to follow the hidden steps behind entering a car. Player and vehicle own separate positions; `Mode` chooses which receives input. Distance checks enable interactions; collision checks validate exit positions.

## Deliberate v0 limits

- One finite 208 x 208 m neighborhood, one car, one room. No streaming or infinite terrain.
- Arcade driving with a conservative circular collider. No suspension, damage, realistic tires, or slopes.
- Planar walls plus a vertical jump arc. No climbing or standing on roofs/furniture. Palm trees and surface decorations are visual only.
- Surface swimming with slower movement. No diving, drowning, stamina, buoyancy, or wave physics. Cars stop at the shore.
- The studio is a scene transition. No entrance animation; other buildings are solid shells.
- Progress lasts for the current session. No saves, audio, NPCs, police, combat, missions, money, stores, dialogue, or multiplayer.

Buildings use distance culling, not streaming or general occlusion. The camera shortens against solid geometry. The program uses a local system font when available, falling back to raylib's built-in font. Font files are not redistributed.

## Credits

Built on [raylib](https://github.com/raysan5/raylib), pinned to `c1ab645ca298a2801097931d1079b10ff7eb9df8` (5.5). See [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt). No commercial game assets are used.
