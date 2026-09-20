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
void Simulation::setStressNpcs(int n) {
    npcs.clear();
    npcs.reserve(n > 0 ? static_cast<std::size_t>(n) : 0);
    const auto& route = npcRoute();
    float total = 0;
    for (std::size_t i=0;i<route.size();++i) total += length(route[(i+1)%route.size()]-route[i]);
    for (int k=0;k<n;++k) {
        // Deterministic even spacing along the loop perimeter.
        float d = total * static_cast<float>(k) / static_cast<float>(n);
        std::size_t seg = 0;
        float segLen = length(route[1]-route[0]);
        while (d > segLen && seg+1 < route.size()) { d -= segLen; ++seg; segLen = length(route[(seg+1)%route.size()]-route[seg]); }
        Vec2 a = route[seg], b = route[(seg+1)%route.size()];
        Vec2 dir = normalized(b-a);
        Npc npc;
        npc.position = a+dir*d;
        npc.waypoint = (seg+1) % route.size();
        npc.yaw = std::atan2(dir.x, dir.z);
        npc.speed = 1.4f + static_cast<float>(k % 5)*0.1f;
        npcs.push_back(npc);
    }
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
Simulation::Simulation() {
    // V0.3: the second car is pure data registration -- no new interaction
    // code was needed to make it enterable. Parked clear of the NPC route
    // and the studio door.
    vehicles.resize(2);
    vehicles[1].position = {-14, -46};
    vehicles[1].yaw = Pi/2;
}
void Simulation::reset() { *this = Simulation{}; }
void Simulation::say(const std::string& text) { notice=text; noticeTime=3; }
Vehicle* Simulation::driven() { return drivenVehicle >= 0 ? &vehicles[drivenVehicle] : nullptr; }
const Vehicle* Simulation::driven() const { return drivenVehicle >= 0 ? &vehicles[drivenVehicle] : nullptr; }
Vec2 Simulation::focus() const { return mode == Mode::Driving ? driven()->position : player.position; }
bool Simulation::freePosition(Vec2 p, float r, bool interior, int selfVehicle) const {
    float bound = interior ? 10.0f : World::Limit;
    if (p.x-r < -bound || p.x+r > bound || p.z-r < -bound || p.z+r > bound) return false;
    if (selfVehicle >= 0 && p.x+r > World::WaterEdge-2) return false;
    for (const auto& b : interior ? world.room : world.buildings) {
        ++collisionChecks;
        if (overlaps(p,r,b)) return false;
    }
    // Every parked car is a physical object; a moving vehicle skips only itself.
    if (!interior) for (std::size_t i=0;i<vehicles.size();++i)
        if (static_cast<int>(i) != selfVehicle && length(p-vehicles[i].position) < r+Vehicle::Radius) return false;
    return true;
}
bool Simulation::move(Vec2& p, Vec2 delta, float radius, bool interior, int selfVehicle) {
    // Short, axis-separated steps prevent tunneling and allow sliding along walls.
    int count = std::max(1, static_cast<int>(std::ceil(length(delta)/0.12f)));
    Vec2 d = delta*(1.0f/static_cast<float>(count));
    bool hit = false;
    for (int i=0; i<count; ++i) {
        Vec2 next{p.x+d.x,p.z};
        if (freePosition(next,radius,interior,selfVehicle)) p=next; else hit=true;
        next={p.x,p.z+d.z};
        if (freePosition(next,radius,interior,selfVehicle)) p=next; else hit=true;
    }
    return hit;
}
bool Simulation::canEnterCar(const Vehicle& v) const {
    if (mode!=Mode::OnFoot || player.swimming || player.height>0.05f || length(player.position-v.position)>=4.5f) return false;
    // Proximity alone is insufficient when a building corner lies between us.
    for (int i=0;i<=30;++i) {
        Vec2 p=player.position+(v.position-player.position)*(i/30.0f);
        for (const Box& b : world.buildings) if (overlaps(p,0.1f,b)) return false;
    }
    return true;
}
void Simulation::enterVehicle(std::size_t i) {
    mode=Mode::Driving; drivenVehicle=static_cast<int>(i);
    player.position=vehicles[i].position;
    progress.enteredCar=true;
    say("W / S to accelerate and reverse. Space to brake.");
}
std::vector<Interactable> Simulation::interactables() const {
    // Registration order is priority order: vehicles, then studio, then NPC.
    std::vector<Interactable> out;
    if (mode == Mode::Interior) {
        out.push_back({[this]{ return world.roomExit; }, 2.4f,
            [](const Simulation&){ return true; },
            [](Simulation& s){
                Vec2 offsets[]={{0,1.5f},{-3.5f,1.5f},{3.5f,1.5f},{0,5.0f}};
                for (Vec2 offset : offsets) {
                    Vec2 exit=s.world.entrance+offset;
                    if (!s.freePosition(exit,0.45f,false)) continue;
                    s.player.position=exit; s.mode=Mode::OnFoot;
                    s.player.yaw=0; s.say("Welcome back to Palm District."); return;
                }
                s.say("The street exit is blocked. Reset with R to return to spawn.");
            },
            [](const Simulation&){ return "E  /  Return to street"; }});
        return out;
    }
    if (mode != Mode::OnFoot) return out;
    for (std::size_t i=0;i<vehicles.size();++i) {
        out.push_back({[this,i]{ return vehicles[i].position; }, 4.5f,
            [i](const Simulation& s){ return s.canEnterCar(s.vehicles[i]); },
            [i](Simulation& s){ s.enterVehicle(i); },
            [](const Simulation&){ return "E  /  Enter vehicle"; }});
    }
    out.push_back({[this]{ return world.entrance; }, 2.8f,
        [](const Simulation&){ return true; },
        [](Simulation& s){
            s.mode=Mode::Interior; s.player.position={0,6.5f}; s.player.yaw=Pi;
            s.progress.visitedRoom=true; s.say("Studio interior. The mint door leads outside.");
        },
        [](const Simulation&){ return "E  /  Enter the studio"; }});
    if (!npcs.empty()) {
        out.push_back({[this]{ return npcs[0].position; }, 2.2f,
            [](const Simulation&){ return true; },
            [](Simulation& s){
                // V0.3 light touch: the pedestrian acknowledges the player.
                // Dialogue trees wait for a later milestone.
                Npc& n=s.npcs[0];
                Vec2 to=s.player.position-n.position;
                n.yaw=std::atan2(to.x,to.z);
                n.state=NpcState::Idle; n.idleTimer=2.0f;
                s.say("The pedestrian nods hello.");
            },
            [](const Simulation&){ return "E  /  Greet pedestrian"; }});
    }
    return out;
}
std::string Simulation::prompt() const {
    if (mode == Mode::Driving) {
        const Vehicle* v=driven();
        return (v && std::abs(v->speed) < 1) ? "E  /  Leave vehicle" : "Brake to exit";
    }
    if (player.swimming) return "Swim toward the beach to return to shore";
    for (const auto& it : interactables()) {
        if (length(player.position-it.position()) > it.radius) continue;
        if (!it.eligible(*this)) continue;
        return it.prompt(*this);
    }
    if (mode == Mode::Interior)
        return "Explore the room. Exit at the mint door.";
    return "Explore freely. Find the car, studio, and waterfront.";
}
void Simulation::interact() {
    if (mode == Mode::Driving) {
        Vehicle& v=*driven();
        if (std::abs(v.speed) >= 1) { say("Stop the car before getting out."); return; }
        Vec2 f=forward(v.yaw), right{f.z,-f.x};
        // Prefer the driver's side, then try other sides. Never exit inside a wall.
        Vec2 exits[] = {v.position-right*3.5f,v.position+right*3.5f,
                        v.position-f*3.8f,v.position+f*3.8f};
        for (Vec2 p : exits) if (p.x < World::WaterEdge && freePosition(p,0.45f,false)) {
            player.position=p; player.height=0; player.verticalSpeed=0;
            player.swimming=false; mode=Mode::OnFoot; drivenVehicle=-1; v.speed=0;
            say("Back on foot."); return;
        }
        say("No room to exit. Move the car into an open space."); return;
    }
    if (player.height > 0.05f || player.swimming) return;
    for (const auto& it : interactables()) {
        if (length(player.position-it.position()) > it.radius) continue;
        if (!it.eligible(*this)) continue;
        it.trigger(*this);
        return;
    }
}
void Simulation::step(const Input& in, float dt) {
    if (!std::isfinite(dt) || dt <= 0) return;
    dt=std::min(dt,0.1f);
    time+=dt; noticeTime=std::max(0.0f,noticeTime-dt); collided=false; justLanded=false;
    collisionChecks=0;
    if (in.interact) interact();
    if (mode == Mode::Driving) {
        Vehicle& v=*driven();
        float throttle=std::clamp(in.throttle,-1.0f,1.0f);
        float steer=std::clamp(in.steering,-1.0f,1.0f);
        v.steering += (steer-v.steering)*std::min(1.0f,dt*9);
        if (in.brake) {
            float reduction=30*dt;
            v.speed=std::copysign(std::max(0.0f,std::abs(v.speed)-reduction),v.speed);
        } else if (std::abs(throttle) > 0.01f) {
            v.speed += throttle*(v.speed*throttle < 0 ? 24.0f : 12.0f)*dt;
        } else {
            v.speed *= std::exp(-0.8f*dt);
            if (std::abs(v.speed)<0.03f) v.speed=0;
        }
        v.speed=std::clamp(v.speed,-10.0f,30.0f);
        // V0.1: keep the original high-speed yaw rate, but add a low-speed
        // authority term so parking-lot maneuvering doesn't feel dead. The
        // Gaussian decays to ~0 by 24 m/s, leaving highway steering untouched.
        float authority=std::abs(v.speed)*0.065f+0.9f*std::exp(-(v.speed*v.speed)/(2*8.0f*8.0f));
        v.yaw += v.steering*(v.speed >= 0 ? 1.0f : -1.0f)*authority*dt;
        Vec2 old=v.position;
        collided=move(v.position,forward(v.yaw)*(v.speed*dt),Vehicle::Radius,false,drivenVehicle);
        if (collided) v.speed=0;
        progress.driven+=length(v.position-old); player.position=v.position;
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
