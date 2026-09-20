#pragma once
#include <functional>
#include <string>
#include <vector>

namespace palm {
constexpr float Pi = 3.14159265359f;
constexpr float FixedStep = 1.0f / 120.0f;
struct Vec2 { float x = 0, z = 0; };
Vec2 operator+(Vec2 a, Vec2 b);
Vec2 operator-(Vec2 a, Vec2 b);
Vec2 operator*(Vec2 a, float s);
float length(Vec2 v);
Vec2 normalized(Vec2 v);
Vec2 forward(float yaw);
struct Box {
    float x, z, width, depth, height;
    int palette = 0;
};
bool overlaps(Vec2 center, float radius, const Box& box);
enum class Mode { OnFoot, Driving, Interior };
struct Player {
    Vec2 position{7, 12};
    float height = 0, verticalSpeed = 0, yaw = Pi;
    bool swimming = false;
};
struct Vehicle {
    Vec2 position{2, 5};
    float yaw = Pi, speed = 0, steering = 0;
    static constexpr float Radius = 2.45f;
};
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
    std::vector<Box> buildings;
    std::vector<Box> room;
    Vec2 entrance{26, -9.6f};
    Vec2 roomExit{0, 7.8f};
    static constexpr float Limit = 103;
    static constexpr float WaterEdge = 74;
    World();
};
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
    bool freePosition(Vec2 p, float radius, bool interior, int selfVehicle = -1) const;
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
    bool move(Vec2& p, Vec2 delta, float radius, bool interior, int selfVehicle = -1);
    void updateNpcs(float dt);
    void say(const std::string& text);
};
} // namespace palm
