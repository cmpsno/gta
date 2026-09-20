#pragma once
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
class Simulation {
public:
    World world;
    Player player;
    Vehicle car;
    Mode mode = Mode::OnFoot;
    Progress progress;
    std::string notice;
    float noticeTime = 0, time = 0;
    bool collided = false;
    // V0.1 feel feedback: set for one step when the player lands a jump,
    // and while sprint input is actually moving the player.
    bool justLanded = false, sprinting = false;

    void reset();
    void step(const Input& input, float dt);
    void interact();
    std::string prompt() const;
    bool freePosition(Vec2 p, float radius, bool interior, bool vehicle = false) const;
    Vec2 focus() const;
private:
    bool canEnterCar() const;
    bool move(Vec2& p, Vec2 delta, float radius, bool interior, bool vehicle = false);
    void say(const std::string& text);
};
} // namespace palm
