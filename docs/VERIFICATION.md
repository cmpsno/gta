# V0 verification

Verified on Windows on 2026-09-15.

## Results

- Release graphical build: passed with GCC 16.1.0, C++17, CMake/Ninja, raylib 5.5.
- Headless simulation suite: **46 assertions passed** via CTest and direct execution.
- Separate `BUILD_GAME=OFF` configuration: built and passed CTest without raylib.
- OpenGL rendering: **five scenes passed**, each running 120 frames: street, driving, studio, water, welcome.
- Captured images were visually inspected for scene geometry, player/vehicle visibility, and readable interface layout.
- The executable imports Windows system libraries only; it does not require raylib, GCC, or C++ runtime DLLs beside it.
- Git whitespace check for game code and new documentation: passed. The verbatim original notes and upstream license retain their existing trailing whitespace.

The render environment reported Intel HD Graphics 630 and OpenGL 3.3. Screenshot FPS counters are momentary values from hidden smoke runs, not a performance benchmark or a frame-rate guarantee.

## Behavior covered

Spawn clearance; walking speed; diagonal normalization; sprint speed; jump arc and landing; wall sliding; wall and world boundaries; car proximity; grounded entry; blocked line of approach; transfer of control; acceleration; reverse; steering; braking; blocked and moving exits; safe studio return around a parked car; room walls and entry/exit proximity; water speed and shoreline recovery; vehicle shoreline protection; reset; simulation step rate; rejection of invalid time values.

## Limits of verification

Automated rendering and simulation checks do not replace a human keyboard/mouse playtest. Camera and vehicle feel should be tuned from playtesting. Linux/macOS portability and Visual Studio compilation have not been verified. The chosen raylib custom build emits upstream macro-redefinition warnings; game sources compiled successfully without warnings. There is no claim of realistic physics, full-world streaming, or production readiness.
