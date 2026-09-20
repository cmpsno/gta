#pragma once
#include "simulation.h"
#include "raylib.h"

namespace palm {
struct View {
    Camera3D camera{};
    float yaw = Pi, pitch = 0.38f, distance = 11;
    bool help = true, debug = false, paused = false, welcome = true;
    bool moving = false;
    float shake = 0; // V0.1: landing shake, decays exponentially in updateCamera.
    double simMs = 0, renderMs = 0; // V0.4: per-frame timing, EMA-smoothed in main.
    Font font{};
};
void updateCamera(View& view, const Simulation& sim, float dt, bool snap = false);
void drawScene(const Simulation& sim, const View& view);
void drawHud(const Simulation& sim, const View& view);
} // namespace palm
