#include "simulation.h"
#include <algorithm>
#include <cmath>

namespace palm {
Vec2 operator+(Vec2 a, Vec2 b) { return {a.x+b.x, a.z+b.z}; }
Vec2 operator-(Vec2 a, Vec2 b) { return {a.x-b.x, a.z-b.z}; }
Vec2 operator*(Vec2 a, float s) { return {a.x*s, a.z*s}; }
float length(Vec2 v) { return std::sqrt(v.x*v.x+v.z*v.z); }
Vec2 normalized(Vec2 v) { float n = length(v); return n > 0.0001f ? v*(1/n) : Vec2{}; }
Vec2 forward(float yaw) { return {std::sin(yaw), std::cos(yaw)}; }
bool overlaps(Vec2 p, float r, const Box& b) {
    float x = std::clamp(p.x, b.x-b.width/2, b.x+b.width/2);
    float z = std::clamp(p.z, b.z-b.depth/2, b.z+b.depth/2);
    return (p.x-x)*(p.x-x)+(p.z-z)*(p.z-z) < r*r;
}
int Progress::completed() const {
    return (walked >= 20) + jumped + enteredCar + (driven >= 80) + visitedRoom + swam;
}
// Sidewalk loop around the central block: x=±8 rides the edge of the north-
// south road at x=0, z=±40 crosses between buildings. All four points and
// every segment were checked against World::buildings by hand.
const std::vector<Vec2>& npcRoute() {
    static const std::vector<Vec2> route{{8,-40},{8,40},{-8,40},{-8,-40}};
    return route;
}
void Simulation::updateNpcs(float dt) {
    const auto& route = npcRoute();
    for (auto& n : npcs) {
        if (n.state == NpcState::Idle) {
            n.idleTimer -= dt;
            if (n.idleTimer <= 0) {
                n.waypoint = (n.waypoint+1) % route.size();
                n.state = NpcState::Walking;
            }
            continue;
        }
        Vec2 target = route[n.waypoint % route.size()];
        Vec2 to = target - n.position;
        if (length(to) < 0.6f) { n.state = NpcState::Idle; n.idleTimer = 2.5f; continue; }
        Vec2 dir = normalized(to);
        n.yaw = std::atan2(dir.x, dir.z);
        // Same axis-separated step-and-resolve path as the player: one
        // collision implementation for every walker.
        move(n.position, dir*(n.speed*dt), 0.45f, false);
    }
}
World::World() {
    // A finite, hand-authored neighborhood. Every building is also a collider.
    buildings = {
        {-30,-29,28,30,17,0}, {26,-25,26,28,9,1},
        {-30,29,28,28,11,2}, {28,30,28,28,21,3},
        {-80,-29,20,30,24,3}, {-80,29,20,28,14,1},
        {-30,-80,28,22,27,2}, {27,-80,28,22,15,0},
        {-80,-80,20,22,18,0}, {-80,80,20,22,22,2},
        {-30,80,28,22,16,1}, {28,80,28,22,10,0}
    };
    room = {{-10,0,1,20,5,0}, {10,0,1,20,5,0}, {0,-10,21,1,5,0},
            {0,10,21,1,5,0}, {-5,-5,3,2,1.4f,1}, {5,-4,3,6,1,2}};
}
void Simulation::reset() { *this = Simulation{}; }
void Simulation::say(const std::string& text) { notice=text; noticeTime=3; }
Vec2 Simulation::focus() const { return mode == Mode::Driving ? car.position : player.position; }
bool Simulation::freePosition(Vec2 p, float r, bool interior, bool vehicle) const {
    float bound = interior ? 10.0f : World::Limit;
    if (p.x-r < -bound || p.x+r > bound || p.z-r < -bound || p.z+r > bound) return false;
    if (vehicle && p.x+r > World::WaterEdge-2) return false;
    for (const auto& b : interior ? world.room : world.buildings)
        if (overlaps(p,r,b)) return false;
    // The parked car remains a physical object while the player is on foot.
    if (!vehicle && !interior && length(p-car.position) < r+Vehicle::Radius) return false;
    return true;
}
bool Simulation::move(Vec2& p, Vec2 delta, float radius, bool interior, bool vehicle) {
    // Short, axis-separated steps prevent tunneling and allow sliding along walls.
    int count = std::max(1, static_cast<int>(std::ceil(length(delta)/0.12f)));
    Vec2 d = delta*(1.0f/static_cast<float>(count));
    bool hit = false;
    for (int i=0; i<count; ++i) {
        Vec2 next{p.x+d.x,p.z};
        if (freePosition(next,radius,interior,vehicle)) p=next; else hit=true;
        next={p.x,p.z+d.z};
        if (freePosition(next,radius,interior,vehicle)) p=next; else hit=true;
    }
    return hit;
}
bool Simulation::canEnterCar() const {
    if (mode!=Mode::OnFoot || player.swimming || player.height>0.05f || length(player.position-car.position)>=4.5f) return false;
    // Proximity alone is insufficient when a building corner lies between us.
    for (int i=0;i<=30;++i) {
        Vec2 p=player.position+(car.position-player.position)*(i/30.0f);
        for (const Box& b : world.buildings) if (overlaps(p,0.1f,b)) return false;
    }
    return true;
}
std::string Simulation::prompt() const {
    if (mode == Mode::Driving) return std::abs(car.speed) < 1 ? "E  /  Leave vehicle" : "Brake to exit";
    if (mode == Mode::Interior)
        return length(player.position-world.roomExit) < 2.4f ? "E  /  Return to street" : "Explore the room. Exit at the mint door.";
    if (player.swimming) return "Swim toward the beach to return to shore";
    if (canEnterCar()) return "E  /  Enter vehicle";
    if (length(player.position-world.entrance) < 2.8f) return "E  /  Enter the studio";
    return "Explore freely. Find the car, studio, and waterfront.";
}
void Simulation::interact() {
    if (mode == Mode::Driving) {
        if (std::abs(car.speed) >= 1) { say("Stop the car before getting out."); return; }
        Vec2 f=forward(car.yaw), right{f.z,-f.x};
        // Prefer the driver's side, then try other sides. Never exit inside a wall.
        Vec2 exits[] = {car.position-right*3.5f,car.position+right*3.5f,
                        car.position-f*3.8f,car.position+f*3.8f};
        for (Vec2 p : exits) if (p.x < World::WaterEdge && freePosition(p,0.45f,false)) {
            player.position=p; player.height=0; player.verticalSpeed=0;
            player.swimming=false; mode=Mode::OnFoot; car.speed=0;
            say("Back on foot."); return;
        }
        say("No room to exit. Move the car into an open space."); return;
    }
    if (player.height > 0.05f || player.swimming) return;
    if (mode == Mode::Interior) {
        if (length(player.position-world.roomExit) < 2.4f) {
            Vec2 offsets[]={{0,1.5f},{-3.5f,1.5f},{3.5f,1.5f},{0,5.0f}};
            for (Vec2 offset : offsets) {
                Vec2 exit=world.entrance+offset;
                if (!freePosition(exit,0.45f,false)) continue;
                player.position=exit; mode=Mode::OnFoot;
                player.yaw=0; say("Welcome back to Palm District."); return;
            }
            say("The street exit is blocked. Reset with R to return to spawn.");
        }
        return;
    }
    if (canEnterCar()) {
        mode=Mode::Driving; player.position=car.position; progress.enteredCar=true;
        say("W / S to accelerate and reverse. Space to brake.");
    } else if (length(player.position-world.entrance) < 2.8f) {
        mode=Mode::Interior; player.position={0,6.5f}; player.yaw=Pi;
        progress.visitedRoom=true; say("Studio interior. The mint door leads outside.");
    }
}
void Simulation::step(const Input& in, float dt) {
    if (!std::isfinite(dt) || dt <= 0) return;
    dt=std::min(dt,0.1f);
    time+=dt; noticeTime=std::max(0.0f,noticeTime-dt); collided=false; justLanded=false;
    if (in.interact) interact();
    if (mode == Mode::Driving) {
        float throttle=std::clamp(in.throttle,-1.0f,1.0f);
        float steer=std::clamp(in.steering,-1.0f,1.0f);
        car.steering += (steer-car.steering)*std::min(1.0f,dt*9);
        if (in.brake) {
            float reduction=30*dt;
            car.speed=std::copysign(std::max(0.0f,std::abs(car.speed)-reduction),car.speed);
        } else if (std::abs(throttle) > 0.01f) {
            car.speed += throttle*(car.speed*throttle < 0 ? 24.0f : 12.0f)*dt;
        } else {
            car.speed *= std::exp(-0.8f*dt);
            if (std::abs(car.speed)<0.03f) car.speed=0;
        }
        car.speed=std::clamp(car.speed,-10.0f,30.0f);
        // V0.1: keep the original high-speed yaw rate, but add a low-speed
        // authority term so parking-lot maneuvering doesn't feel dead. The
        // Gaussian decays to ~0 by 24 m/s, leaving highway steering untouched.
        float authority=std::abs(car.speed)*0.065f+0.9f*std::exp(-(car.speed*car.speed)/(2*8.0f*8.0f));
        car.yaw += car.steering*(car.speed >= 0 ? 1.0f : -1.0f)*authority*dt;
        Vec2 old=car.position;
        collided=move(car.position,forward(car.yaw)*(car.speed*dt),Vehicle::Radius,false,true);
        if (collided) car.speed=0;
        progress.driven+=length(car.position-old); player.position=car.position;
        updateNpcs(dt);
        return;
    }
    bool interior=mode==Mode::Interior;
    player.swimming=!interior && player.position.x > World::WaterEdge && player.height <= 0;
    Vec2 direction=length(in.movement)>1 ? normalized(in.movement) : in.movement;
    sprinting = in.sprint && length(direction) > 0.01f && !player.swimming;
    float speed=player.swimming ? 3.0f : (in.sprint ? 9.0f : 4.5f);
    Vec2 old=player.position;
    collided=move(player.position,direction*(speed*dt),0.45f,interior);
    progress.walked+=length(player.position-old);
    if (length(direction)>0.01f) player.yaw=std::atan2(direction.x,direction.z);
    if (in.jump && !player.swimming && player.height<=0) {
        player.verticalSpeed=7; progress.jumped=true;
    }
    player.verticalSpeed-=20*dt;
    player.height=std::max(0.0f,player.height+player.verticalSpeed*dt);
    if (player.height<=0) {
        // V0.1: a real fall (not a step or a swim stroke) flags one frame of
        // landing feedback for the renderer.
        if (player.verticalSpeed < -3.5f) justLanded=true;
        player.verticalSpeed=0;
    }
    player.swimming=!interior && player.position.x > World::WaterEdge && player.height<=0;
    if (player.swimming) progress.swam=true;
    updateNpcs(dt);
}
} // namespace palm
