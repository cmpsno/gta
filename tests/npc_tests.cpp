#include "simulation.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace palm;
int assertions=0;
void check(bool ok,const std::string& message) {
    ++assertions;
    if (!ok) throw std::runtime_error(message);
}
void run(Simulation& s,const Input& input,int steps) { for (int i=0;i<steps;++i) s.step(input,FixedStep); }
void near(float a,float b,float tolerance,const char* message) {check(std::abs(a-b)<tolerance,message);}
int main() {
    try {
        Simulation s;
        // One NPC exists, walking its route, and it spawns on clear ground.
        check(s.npcs.size()==1,"V0.2 ships exactly one NPC");
        check(s.npcs[0].state==NpcState::Walking,"NPC starts walking");
        check(s.freePosition(playerAABB({s.npcs[0].position.x, 0, s.npcs[0].position.z}),false),"NPC spawn must be clear");
        near(s.npcs[0].position.x,npcRoute()[0].x,0.001f,"NPC spawns on route point 0");
        near(s.npcs[0].position.z,npcRoute()[0].z,0.001f,"NPC spawns on route point 0");
        // The NPC visits every waypoint in route order, pausing at each.
        std::vector<std::size_t> order;
        NpcState prev=s.npcs[0].state;
        int pauses=0;
        for (int i=0;i<40000 && order.size()<4;++i) {
            s.step({},FixedStep);
            NpcState now=s.npcs[0].state;
            if (now==NpcState::Idle && prev==NpcState::Walking) {
                order.push_back(s.npcs[0].waypoint);
                ++pauses;
                near(length(s.npcs[0].position-npcRoute()[s.npcs[0].waypoint]),0,0.7f,
                     "NPC pauses at the waypoint it reached");
            }
            prev=now;
        }
        check(order==std::vector<std::size_t>({1,2,3,0}),"NPC visits waypoints in route order");
        check(pauses==4,"NPC pauses at every waypoint");
        // Idle lasts ~2.5 s, then the NPC resumes toward the next waypoint.
        s.reset();
        Npc& n=s.npcs[0];
        n.position={8,39.5f};n.waypoint=1;n.state=NpcState::Walking;n.idleTimer=0;
        s.step({},FixedStep);
        check(n.state==NpcState::Idle,"NPC idles on reaching a waypoint");
        check(n.idleTimer>2.0f && n.idleTimer<=2.5f,"Idle timer starts at ~2.5 s");
        run(s,{},120);
        check(n.state==NpcState::Idle,"Idle persists while the timer runs");
        run(s,{},300);
        check(n.state==NpcState::Walking && n.waypoint==2,"Idle expiry advances to the next waypoint");
        // A wall across the route stops the NPC; it never penetrates geometry.
        s.reset();
        s.world.buildings.push_back(makeBlock(8,0,4,6,8,0));
        run(s,{},3000);
        check(s.npcs[0].position.z<-2.0f,"NPC cannot walk through a blocking wall");
        check(s.npcs[0].position.z>-40.0f,"NPC still approaches until blocked");
        check(s.freePosition(playerAABB({s.npcs[0].position.x, 0, s.npcs[0].position.z}),false),"Blocked NPC never penetrates geometry");
        // The NPC keeps walking while the player drives, swims, or idles.
        s.reset();
        Vec2 before=s.npcs[0].position;
        run(s,{},600);
        check(length(s.npcs[0].position-before)>1.0f,"NPC walks independently of player input");
        std::cout<<"PASS: "<<assertions<<" npc assertions\n";return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
