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
        Vec3 npcCenter{n.position.x, Player::Height/2, n.position.z};
        moveBody(npcCenter, {Player::Radius, Player::Height/2, Player::Radius},
                 dir*(n.speed*dt), false);
        n.position = planar(npcCenter);
    }
}
World::World() {
    // A finite, hand-authored neighborhood. Every building is also a collider.
    buildings = {
        makeBlock(-30,-29,28,30,17,0), makeBlock(26,-25,26,28,9,1),
        makeBlock(28,30,28,28,21,3),
        makeBlock(-80,-29,20,30,24,3), makeBlock(-80,29,20,28,14,1),
        makeBlock(-30,-80,28,22,27,2), makeBlock(27,-80,28,22,15,0),
        makeBlock(-80,-80,20,22,18,0), makeBlock(-80,80,20,22,22,2),
        makeBlock(-30,80,28,22,16,1), makeBlock(28,80,28,22,10,0)
    };
    room = {makeBlock(-10,0,1,20,5,0), makeBlock(10,0,1,20,5,0), makeBlock(0,-10,21,1,5,0),
            makeBlock(0,10,21,1,5,0), makeBlock(-5,-5,3,2,1.4f,1), makeBlock(5,-4,3,6,1,2)};
    // The ground as an explicit slab, spanning the playable area (and the
    // interior's coordinate range). Everything reachable stands on it until
    // later tasks add raised floors.
    floors = {{{-104, -1, -104}, {104, 0, 104}}};
    // Phase 1 task 5: test staircase. Five 0.5 m steps climbing +z to a
    // 2.5 m platform. Clear of buildings, spawn, and the NPC route.
    for (int i = 0; i < 5; ++i) {
        float top = 0.5f * (i + 1);
        floors.push_back({{-14, 0, 20 + 2.0f * i}, {-10, top, 22 + 2.0f * i}});
    }
    floors.push_back({{-14, 0, 30}, {-10, 2.5f, 38}}); // platform
    // Phase 1 task 7: two-floor building at (-30, 29). The solid block is
    // replaced by a hollow shell: ground-floor walls, an interior staircase,
    // a second-floor slab, and second-floor walls. Walls live in `buildings`
    // (3D colliders — task 6 makes them block only the floor they're on);
    // the slab and stairs live in `floors` (walkable, step-up climbs them).
    // Footprint x∈[-44,-16], z∈[15,43]; wall thickness 0.6; floor height 3.
    // Phase 1 task 8: the east ground-floor wall has a 2 m doorway at
    // z∈[28,30]. The building is in the exterior coordinate space, so entry
    // is walk-through — no teleport. (Phase 2 will generalize Mode::Interior
    // to "inside building X".)
    {
        float x0=-44, x1=-16, z0=15, z1=43, t=0.6f, fh=3.0f;
        float dz0=28, dz1=30; // doorway z-range
        int pal=2;
        // Ground-floor walls (y 0..3). East wall is split for the doorway.
        buildings.push_back({{{x0,0,z0},{x1,fh,z0+t}}, pal});
        buildings.push_back({{{x0,0,z1-t},{x1,fh,z1}}, pal});
        buildings.push_back({{{x0,0,z0},{x0+t,fh,z1}}, pal});
        buildings.push_back({{{x1-t,0,z0},{x1-t,fh,dz0}}, pal});
        buildings.push_back({{{x1-t,0,dz1},{x1-t,fh,z1}}, pal});
        // Second-floor walls (y 3..6).
        buildings.push_back({{{x0,fh,z0},{x1,2*fh,z0+t}}, pal});
        buildings.push_back({{{x0,fh,z1-t},{x1,2*fh,z1}}, pal});
        buildings.push_back({{{x0,fh,z0},{x0+t,2*fh,z1}}, pal});
        buildings.push_back({{{x1-t,fh,z0},{x1,2*fh,z1}}, pal});
        // Second-floor slab (top at y=3), interior.
        floors.push_back({{x0+t, fh-0.3f, z0+t}, {x1-t, fh, z1-t}});
        // Interior staircase: six 0.5 m steps along the west wall, z 18..30.
        for (int i=0;i<6;++i) {
            float top=0.5f*(i+1);
            floors.push_back({{x0+t, 0, 18+2.0f*i}, {x0+t+2.0f, top, 20+2.0f*i}});
        }
    }
}
float findFloorY(Vec3 pos, const std::vector<AABB>& floors) {
    float best = 0.0f;
    for (const auto& f : floors) {
        bool inside = pos.x >= f.min.x && pos.x <= f.max.x &&
                      pos.z >= f.min.z && pos.z <= f.max.z;
        if (inside && f.max.y <= pos.y && f.max.y > best) best = f.max.y;
    }
    return best;
}
float findStepTop(Vec3 pos, const std::vector<AABB>& floors, float maxStep) {
    float best = pos.y + maxStep + 1.0f;
    for (const auto& f : floors) {
        bool inside = pos.x >= f.min.x && pos.x <= f.max.x &&
                      pos.z >= f.min.z && pos.z <= f.max.z;
        if (inside && f.max.y > pos.y && f.max.y <= pos.y + maxStep && f.max.y < best)
            best = f.max.y;
    }
    return best <= pos.y + maxStep ? best : pos.y;
}
Simulation::Simulation() {
    // V0.3: the second car is pure data registration -- no new interaction
    // code was needed to make it enterable. Parked clear of the NPC route
    // and the studio door.
    vehicles.resize(2);
    vehicles[1].position = {-14, 0, -46};
    vehicles[1].yaw = Pi/2;
}
void Simulation::reset() { *this = Simulation{}; }
void Simulation::say(const std::string& text) { notice=text; noticeTime=3; }
Vehicle* Simulation::driven() { return drivenVehicle >= 0 ? &vehicles[drivenVehicle] : nullptr; }
const Vehicle* Simulation::driven() const { return drivenVehicle >= 0 ? &vehicles[drivenVehicle] : nullptr; }
Vec2 Simulation::focus() const { return mode == Mode::Driving ? planar(driven()->position) : planar(player.position); }
bool Simulation::freePosition(const AABB& body, bool interior, int selfVehicle) const {
    // Phase 1 task 6: movers are vertical cylinders. The XZ footprint is the
    // AABB's inscribed circle (every mover is square on XZ, and a circle is
    // rotation-invariant for yawing bodies like the car), extruded over the
    // AABB's Y range. A block collides only if the circle hits its footprint
    // AND the Y ranges overlap — so a wall on an upper floor no longer
    // blocks the ground floor, and vice versa.
    Vec2 c{(body.min.x+body.max.x)*0.5f, (body.min.z+body.max.z)*0.5f};
    float r = std::min(body.max.x-body.min.x, body.max.z-body.min.z)*0.5f;
    float y0 = body.min.y, y1 = body.max.y;
    float bound = interior ? 10.0f : World::Limit;
    if (c.x-r < -bound || c.x+r > bound || c.z-r < -bound || c.z+r > bound) return false;
    if (selfVehicle >= 0 && c.x+r > World::WaterEdge-2) return false;
    for (const auto& b : interior ? world.room : world.buildings) {
        ++collisionChecks;
        const AABB& bb = b.bounds;
        if (y1 <= bb.min.y || y0 >= bb.max.y) continue;
        float x = std::clamp(c.x, bb.min.x, bb.max.x);
        float z = std::clamp(c.z, bb.min.z, bb.max.z);
        if ((c.x-x)*(c.x-x)+(c.z-z)*(c.z-z) < r*r) return false;
    }
    // Every parked car is a physical object; a moving vehicle skips only itself.
    if (!interior) for (std::size_t i=0;i<vehicles.size();++i) {
        if (static_cast<int>(i) == selfVehicle) continue;
        const AABB ob = vehicleAABB(vehicles[i].position);
        if (y1 <= ob.min.y || y0 >= ob.max.y) continue;
        if (length(c-planar(vehicles[i].position)) < r+Vehicle::Radius) return false;
    }
    return true;
}
bool Simulation::moveBody(Vec3& center, Vec3 halfExtents, Vec2 delta, bool interior, int selfVehicle) {
    // Short, axis-separated steps prevent tunneling and allow sliding along
    // walls. Only X and Z step; Y is carried through and resolved by the
    // floor/gravity system, so 3D overlap is tested on every substep.
    auto at = [&](Vec3 c) { return AABB{c-halfExtents, c+halfExtents}; };
    int count = std::max(1, static_cast<int>(std::ceil(length(delta)/0.12f)));
    Vec2 d = delta*(1.0f/static_cast<float>(count));
    bool hit = false;
    for (int i=0; i<count; ++i) {
        Vec3 next{center.x+d.x, center.y, center.z};
        if (freePosition(at(next),interior,selfVehicle)) center=next; else hit=true;
        next={center.x, center.y, center.z+d.z};
        if (freePosition(at(next),interior,selfVehicle)) center=next; else hit=true;
    }
    return hit;
}
bool Simulation::canEnterCar(const Vehicle& v) const {
    if (mode!=Mode::OnFoot || player.swimming || player.position.y>0.05f || length(planar(player.position)-planar(v.position))>=4.5f) return false;
    // Proximity alone is insufficient when a building corner lies between us.
    // The sight check runs at eye height so a low obstacle no longer blocks it.
    for (int i=0;i<=30;++i) {
        Vec2 p=planar(player.position)+(planar(v.position)-planar(player.position))*(i/30.0f);
        AABB eye{{p.x-0.1f, 1.5f, p.z-0.1f}, {p.x+0.1f, 1.7f, p.z+0.1f}};
        for (const Block& b : world.buildings) if (overlaps(eye, b.bounds)) return false;
    }
    return true;
}
void Simulation::enterVehicle(std::size_t i) {
    mode=Mode::Driving; drivenVehicle=static_cast<int>(i);
    player.position.x=vehicles[i].position.x; player.position.z=vehicles[i].position.z;
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
                    if (!s.freePosition(playerAABB({exit.x, 0, exit.z}),false)) continue;
                    s.player.position={exit.x, s.player.position.y, exit.z}; s.mode=Mode::OnFoot;
                    s.player.yaw=0; s.say("Welcome back to Palm District."); return;
                }
                s.say("The street exit is blocked. Reset with R to return to spawn.");
            },
            [](const Simulation&){ return "E  /  Return to street"; }});
        return out;
    }
    if (mode != Mode::OnFoot) return out;
    for (std::size_t i=0;i<vehicles.size();++i) {
        out.push_back({[this,i]{ return planar(vehicles[i].position); }, 4.5f,
            [i](const Simulation& s){ return s.canEnterCar(s.vehicles[i]); },
            [i](Simulation& s){ s.enterVehicle(i); },
            [](const Simulation&){ return "E  /  Enter vehicle"; }});
    }
    out.push_back({[this]{ return world.entrance; }, 2.8f,
        [](const Simulation&){ return true; },
        [](Simulation& s){
            s.mode=Mode::Interior; s.player.position={0, s.player.position.y, 6.5f}; s.player.yaw=Pi;
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
                Vec2 to=planar(s.player.position)-n.position;
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
        if (length(planar(player.position)-it.position()) > it.radius) continue;
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
        Vec2 vp=planar(v.position);
        // Prefer the driver's side, then try other sides. Never exit inside a wall.
        Vec2 exits[] = {vp-right*3.5f,vp+right*3.5f,
                        vp-f*3.8f,vp+f*3.8f};
        for (Vec2 p : exits) if (p.x < World::WaterEdge && freePosition(playerAABB({p.x, 0, p.z}),false)) {
            player.position={p.x, 0, p.z}; player.verticalSpeed=0;
            player.swimming=false; mode=Mode::OnFoot; drivenVehicle=-1; v.speed=0;
            say("Back on foot."); return;
        }
        say("No room to exit. Move the car into an open space."); return;
    }
    if (player.position.y > 0.05f || player.swimming) return;
    for (const auto& it : interactables()) {
        if (length(planar(player.position)-it.position()) > it.radius) continue;
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
        Vec2 old=planar(v.position);
        // Phase 1 task 6: the car moves as a 3D body; X and Z resolve
        // against 3D blocks, Y stays pinned to the ground.
        Vec3 carCenter{v.position.x, Vehicle::Height/2, v.position.z};
        collided=moveBody(carCenter, {Vehicle::Radius, Vehicle::Height/2, Vehicle::Radius},
                          forward(v.yaw)*(v.speed*dt), false, drivenVehicle);
        if (collided) v.speed=0;
        v.position.x=carCenter.x; v.position.z=carCenter.z;
        progress.driven+=length(planar(v.position)-old);
        player.position.x=v.position.x; player.position.z=v.position.z;
        updateNpcs(dt);
        return;
    }
    bool interior=mode==Mode::Interior;
    // Gravity pulls to the nearest floor below (task 4), not to a hardcoded
    // y = 0. Recomputed after the horizontal move since the XZ changed.
    float floorY=findFloorY(player.position, world.floors);
    player.swimming=!interior && player.position.x > World::WaterEdge && player.position.y <= floorY;
    Vec2 direction=length(in.movement)>1 ? normalized(in.movement) : in.movement;
    sprinting = in.sprint && length(direction) > 0.01f && !player.swimming;
    float speed=player.swimming ? 3.0f : (in.sprint ? 9.0f : 4.5f);
    Vec2 old=planar(player.position);
    // Phase 1 task 6: the player walks as a 3D body. X and Z resolve against
    // 3D blocks here; Y resolves in the floor/gravity system below.
    Vec3 bodyCenter{player.position.x, player.position.y + Player::Height/2, player.position.z};
    collided=moveBody(bodyCenter, {Player::Radius, Player::Height/2, Player::Radius},
                      direction*(speed*dt), interior);
    player.position.x=bodyCenter.x; player.position.z=bodyCenter.z;
    progress.walked+=length(planar(player.position)-old);
    if (length(direction)>0.01f) player.yaw=std::atan2(direction.x,direction.z);
    // Phase 1 task 5: step-up. After the horizontal move, if a slab top is
    // within step height above the feet, snap up to it. The reach check is
    // the guard: a slab 2 m up never triggers. This is what lets the player
    // walk up stairs; the planar move itself never sees floors.
    floorY=findFloorY(player.position, world.floors);
    float stepTop=findStepTop(player.position, world.floors, MaxStepHeight);
    if (stepTop > player.position.y) {
        player.position.y=stepTop;
        floorY=stepTop;
    }
    if (in.jump && !player.swimming && player.position.y<=floorY) {
        player.verticalSpeed=7; progress.jumped=true;
    }
    player.verticalSpeed-=20*dt;
    player.position.y=std::max(floorY,player.position.y+player.verticalSpeed*dt);
    if (player.position.y<=floorY) {
        // V0.1: a real fall (not a step or a swim stroke) flags one frame of
        // landing feedback for the renderer.
        if (player.verticalSpeed < -3.5f) justLanded=true;
        player.verticalSpeed=0;
    }
    player.swimming=!interior && player.position.x > World::WaterEdge && player.position.y<=floorY;
    if (player.swimming) progress.swam=true;
    updateNpcs(dt);
}
} // namespace palm
