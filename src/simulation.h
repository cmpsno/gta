#pragma once
#include "vec3.h"
#include <functional>
#include <string>
#include <vector>

namespace palm {
constexpr float Pi = 3.14159265359f;
constexpr float FixedStep = 1.0f / 120.0f;
// Phase 1 task 5: tallest step the player can walk up without jumping.
constexpr float MaxStepHeight = 0.6f;
struct Vec2 { float x = 0, z = 0; };
Vec2 operator+(Vec2 a, Vec2 b);
Vec2 operator-(Vec2 a, Vec2 b);
Vec2 operator*(Vec2 a, float s);
float length(Vec2 v);
Vec2 normalized(Vec2 v);
Vec2 forward(float yaw);
// Phase 1: planar projection. Player/NPC/vehicle collision still resolves on
// the XZ plane until task 6 makes it 3D; this keeps each migration step
// behavior-preserving.
inline Vec2 planar(Vec3 v) { return {v.x, v.z}; }
// Phase 1 task 6: the 3D successor to Box. Collision works on `bounds`;
// `palette` is render-only data carried along.
struct Block {
    AABB bounds;
    int palette = 0;
};
inline Block makeBlock(float x, float z, float w, float d, float h, int palette) {
    return {{{x - w / 2, 0, z - d / 2}, {x + w / 2, h, z + d / 2}}, palette};
}
enum class Mode { OnFoot, Driving, Interior };
// Phase 1 task 2: single Vec3 position. `height` is now `position.y`;
// planar collision still uses `planar(position)` until task 6.
struct Player {
    Vec3 position{7, 0, 12}; // feet
    float verticalSpeed = 0, yaw = Pi;
    bool swimming = false;
    static constexpr float Radius = 0.45f;
    static constexpr float Height = 1.8f;
};
// Body AABBs for the 3D collision in task 6.
inline AABB playerAABB(Vec3 feet) {
    return {{feet.x - Player::Radius, feet.y, feet.z - Player::Radius},
            {feet.x + Player::Radius, feet.y + Player::Height, feet.z + Player::Radius}};
}
// Phase 1 task 3: Vec3 position. The car stays on the ground (y = 0);
// planar collision still uses `planar(position)` until task 6.
struct Vehicle {
    Vec3 position{2, 0, 5};
    float yaw = Pi, speed = 0, steering = 0;
    static constexpr float Radius = 2.45f;
    static constexpr float Height = 1.5f;
};
inline AABB vehicleAABB(Vec3 ground) {
    return {{ground.x - Vehicle::Radius, 0, ground.z - Vehicle::Radius},
            {ground.x + Vehicle::Radius, Vehicle::Height, ground.z + Vehicle::Radius}};
}
// V0.2: one independently-owned pedestrian. Position, facing, speed, route
// progress, and behavior state live here so N NPCs can act independently
// (V0.4 stress test reuses this struct directly).
enum class NpcState { Idle, Walking };
struct Npc {
    Vec2 position{8, -40};
    float yaw = 0;
    std::size_t waypoint = 1; // index into npcRoute() of the current target
    NpcState state = NpcState::Walking;
    float idleTimer = 0;
    float speed = 1.6f;
};
// Hardcoded sidewalk loop on the existing street grid; no nav layer yet.
const std::vector<Vec2>& npcRoute();
struct Input {
    Vec2 movement{}; // World-space direction, derived from camera in main.cpp.
    float throttle = 0, steering = 0;
    bool sprint = false, jump = false, interact = false, brake = false;
};
struct Progress {
    float walked = 0, driven = 0;
    bool jumped = false, enteredCar = false, visitedRoom = false, swam = false;
    int completed() const;
};
struct World {
    std::vector<Block> buildings;
    std::vector<Block> room;
    // Phase 1 task 4: explicit floor slabs. The first entry is the ground
    // itself; later tasks add raised slabs, stairs, and second floors.
    std::vector<AABB> floors;
    Vec2 entrance{26, -9.6f};
    Vec2 roomExit{0, 7.8f};
    static constexpr float Limit = 103;
    static constexpr float WaterEdge = 74;
    World();
};
// Highest slab top at or below pos.y over the player's XZ position.
// Falls back to 0, preserving the old implicit-ground invariant.
float findFloorY(Vec3 pos, const std::vector<AABB>& floors);
// Lowest slab top strictly above pos.y but within maxStep, over the player's
// XZ position. Returns pos.y when no such slab exists. Backs the step-up.
float findStepTop(Vec3 pos, const std::vector<AABB>& floors, float maxStep);
class Simulation; // Interactable callbacks observe/mutate it; defined below.
// V0.3: a proximity interaction with a dynamic anchor, an eligibility check,
// and an effect. Car entry, studio entry/exit, and NPC greeting are all
// instances of this one concept; adding a new interactable is data
// registration in interactables(), not new branches in interact().
struct Interactable {
    std::function<Vec2()> position;
    float radius = 0;
    std::function<bool(const Simulation&)> eligible;
    std::function<void(Simulation&)> trigger;
    std::function<std::string(const Simulation&)> prompt;
};
class Simulation {
public:
    World world;
    Player player;
    std::vector<Vehicle> vehicles;
    std::vector<Npc> npcs = std::vector<Npc>(1); // exactly one through V0.3
    int drivenVehicle = -1; // index into vehicles while Driving, -1 otherwise
    Mode mode = Mode::OnFoot;
    Progress progress;
    std::string notice;
    float noticeTime = 0, time = 0;
    bool collided = false;
    // V0.1 feel feedback: set for one step when the player lands a jump,
    // and while sprint input is actually moving the player.
    bool justLanded = false, sprinting = false;

    Simulation();
    void reset();
    void step(const Input& input, float dt);
    void interact();
    std::string prompt() const;
    // Phase 1 task 6: 3D collision. `body` is the mover's AABB; X and Z
    // resolve axis-by-axis in moveBody, Y resolves in the floor/gravity
    // system. Vehicles are solid to each other; the mover skips itself.
    bool freePosition(const AABB& body, bool interior, int selfVehicle = -1) const;
    bool moveBody(Vec3& center, Vec3 halfExtents, Vec2 delta, bool interior, int selfVehicle = -1);
    Vec2 focus() const;
    Vehicle* driven();
    const Vehicle* driven() const;
    // V0.4: replace the shipped single NPC with N stress walkers distributed
    // along the route. Measurement-only; the game still ships one NPC.
    void setStressNpcs(int n);
    // V0.4: box-overlap tests performed by the last step(); read by the
    // debug overlay and the headless benchmark.
    mutable long collisionChecks = 0;
private:
    bool canEnterCar(const Vehicle& v) const;
    void enterVehicle(std::size_t i);
    std::vector<Interactable> interactables() const;
    void updateNpcs(float dt);
    void say(const std::string& text);
};
} // namespace palm
