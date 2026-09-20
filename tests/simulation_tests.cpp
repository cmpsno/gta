#include "simulation.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

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
        check(s.freePosition(s.player.position,0.45f,false),"Spawn must be clear");
        check(s.freePosition(s.vehicles[0].position,Vehicle::Radius,false,0),"Car spawn must be clear");
        s.player.position={0,30};Input walk;walk.movement={0,-1};
        run(s,walk,120);near(s.player.position.z,25.5f,0.02f,"Walk speed is meters per second");
        Simulation diagonal;diagonal.player.position={0,30};walk.movement={1,-1};
        run(diagonal,walk,120);near(length(diagonal.player.position-Vec2{0,30}),4.5f,0.02f,"Diagonal input must not increase speed");
        s.reset();s.player.position={0,30};walk.movement={0,1};walk.sprint=true;
        run(s,walk,120);near(s.player.position.z,39,0.02f,"Sprint must be faster than walking");
        s.reset();s.player.position={0,30};Input jump;jump.jump=true;s.step(jump,FixedStep);
        check(s.player.height>0,"Jump leaves ground");
        float peak=0;
        for (int i=0;i<120;++i) {s.step({},FixedStep);peak=std::max(peak,s.player.height);}
        check(peak>1 && peak<1.3f,"Jump has a bounded arc");check(s.player.height==0,"Jump lands");
        s.reset();s.player.position={26,-5};walk={};walk.movement={0,-1};walk.sprint=true;
        run(s,walk,600);check(s.player.position.z>=-10.56f,"Building stops the player");
        check(s.freePosition(s.player.position,0.45f,false),"Collision never leaves player penetrating building");
        s.reset();s.player.position={102,40};walk.movement={1,0};run(s,walk,120);
        check(s.player.position.x<=102.55f,"World boundary contains player");
        s.reset();s.interact();check(s.mode==Mode::OnFoot,"Cannot enter a car at arbitrary distance");
        s.player.position={5.2f,5};s.interact();check(s.mode==Mode::Driving,"Nearby car transfers control");
        Input drive;drive.throttle=1;run(s,drive,240);
        check(s.vehicles[0].speed>20 && s.vehicles[0].position.z<0,"Vehicle accelerates and moves");
        near(length(s.player.position-s.vehicles[0].position),0,0.001f,"Player follows occupied car");
        s.interact();check(s.mode==Mode::Driving,"Cannot exit at speed");
        Input brake;brake.brake=true;run(s,brake,240);near(s.vehicles[0].speed,0,0.001f,"Brake stops vehicle");
        s.interact();check(s.mode==Mode::OnFoot,"Stopped car permits exit");
        check(s.freePosition(s.player.position,0.45f,false),"Exit is outside vehicle and world geometry");
        s.reset();s.vehicles[0].position={0,30};s.player.position={3.5f,30};s.interact();
        drive.throttle=-1;run(s,drive,120);check(s.vehicles[0].position.z>30,"Reverse moves backward");
        s.reset();s.player.position={5.2f,5};s.interact();drive={};drive.throttle=1;drive.steering=1;
        run(s,drive,90);check(s.vehicles[0].yaw>Pi,"Steering turns a moving vehicle");
        s.reset();s.vehicles[0].position={26,-5};s.vehicles[0].yaw=Pi;s.player.position={29.5f,-5};s.interact();
        drive={};drive.throttle=1;run(s,drive,1200);
        check(s.vehicles[0].position.z>=-8.56f,"Car cannot tunnel into building at sustained throttle");
        check(s.freePosition(s.vehicles[0].position,Vehicle::Radius,false,0),"Vehicle collider remains clear");
        s.reset();s.vehicles[0].position={68,0};s.vehicles[0].yaw=Pi/2;s.player.position={65,0};s.interact();
        run(s,drive,600);check(s.vehicles[0].position.x<=69.56f,"Water boundary stops car");
        s.reset();s.player.position=s.world.entrance;s.interact();
        check(s.mode==Mode::Interior && s.progress.visitedRoom,"Studio enters its own scene");
        check(s.freePosition(s.player.position,0.45f,true),"Interior spawn is free");
        walk={};walk.movement={0,-1};run(s,walk,600);
        check(s.player.position.z>=-9.06f,"Interior walls contain player");
        s.interact();check(s.mode==Mode::Interior,"Room exit requires proximity");
        s.player.position=s.world.roomExit;s.interact();check(s.mode==Mode::OnFoot,"Room exit returns to street");
        check(s.freePosition(s.player.position,0.45f,false),"Street exit is clear");
        s.reset();s.vehicles[0].position={26,-7};s.player.position={29,-9.6f};
        s.mode=Mode::Interior;s.player.position=s.world.roomExit;s.interact();
        check(s.mode==Mode::OnFoot,"Studio finds an alternate exit around parked car");
        check(s.freePosition(s.player.position,0.45f,false),"Studio exit never overlaps parked car");
        s.reset();s.vehicles[0].position={37,-8.5f};s.player.position={39.5f,-12};
        check(s.freePosition(s.player.position,0.45f,false),"Corner interaction test player is outside wall");
        check(s.freePosition(s.vehicles[0].position,Vehicle::Radius,false,0),"Corner interaction test car is outside wall");
        s.interact();check(s.mode==Mode::OnFoot,"Cannot enter car through building corner");
        s.reset();s.player.position={5.2f,5};s.player.height=0.5f;s.interact();
        check(s.mode==Mode::OnFoot,"Car entry requires grounded player");
        s.reset();s.player.position={26,-10.5f};walk={};walk.movement={1,-1};
        float slideStart=s.player.position.x;run(s,walk,120);
        check(s.player.position.x>slideStart+2,"Diagonal wall contact slides along free axis");
        check(s.player.position.z>=-10.56f,"Sliding does not penetrate wall");
        s.reset();s.player.position={73,30};walk={};walk.movement={1,0};run(s,walk,120);
        check(s.player.swimming && s.progress.swam,"Entering water switches locomotion");
        float start=s.player.position.x;run(s,walk,120);near(s.player.position.x-start,3,0.02f,"Swimming uses its own speed");
        s.step(jump,FixedStep);check(s.player.height==0,"Cannot jump out of deep water");
        walk.movement={-1,0};run(s,walk,360);check(!s.player.swimming,"Shore restores walking");
        // Both exit sides blocked: reject exit rather than teleporting through walls.
        s.reset();s.player.position={5.2f,5};s.interact();
        s.world.buildings.push_back({s.vehicles[0].position.x,s.vehicles[0].position.z,20,20,10,0});
        s.interact();check(s.mode==Mode::Driving,"A fully blocked exit preserves driving state");
        // Compare different simulation call rates and reject invalid time values.
        Simulation a,b;a.player.position={0,30};b.player.position={0,30};walk={};walk.movement={0,1};
        run(a,walk,120);for (int i=0;i<60;++i)b.step(walk,1.0f/60);
        near(a.player.position.z,b.player.position.z,0.01f,"Walking is independent of step rate");
        Vec2 old=a.player.position;a.step(walk,std::numeric_limits<float>::quiet_NaN());a.step(walk,-1);
        near(length(a.player.position-old),0,0.001f,"Invalid time does not corrupt state");
        s.reset();check(s.mode==Mode::OnFoot && s.progress.completed()==0,"Reset restores fresh sandbox");
        // V0.1 feel flags: landing feedback fires once per jump; sprinting tracks input.
        s.reset();Input hj;hj.jump=true;s.step(hj,FixedStep);
        bool sawLanding=false;
        for (int i=0;i<120;++i) {s.step({},FixedStep);if (s.justLanded) sawLanding=true;}
        check(sawLanding,"Landing a jump flags one frame of feedback");
        s.reset();walk={};walk.movement={0,1};walk.sprint=true;s.step(walk,FixedStep);
        check(s.sprinting,"Sprint input while moving flags sprinting");
        walk.sprint=false;s.step(walk,FixedStep);
        check(!s.sprinting,"Releasing sprint clears the flag");
        std::cout<<"PASS: "<<assertions<<" mechanics assertions\n";return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
