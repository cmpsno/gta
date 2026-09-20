#include "simulation.h"
#include <cmath>
#include <iostream>
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
        // Two cars are registered; the second needed no new interaction code.
        check(s.vehicles.size()==2,"Two vehicles are registered");
        check(s.freePosition(s.vehicles[1].position,Vehicle::Radius,false,1),"Second car spawn must be clear");
        check(s.driven()==nullptr,"Nobody is driving at spawn");
        // The second car is enterable through the same generic path as the first.
        s.player.position=s.vehicles[1].position+Vec2{3,0};
        check(s.prompt()=="E  /  Enter vehicle","Second car offers the car prompt");
        s.interact();
        check(s.mode==Mode::Driving && s.drivenVehicle==1,"Entering the second car drives it");
        check(s.driven()==&s.vehicles[1],"driven() resolves to the second car");
        Input drive;drive.throttle=1;run(s,drive,240);
        check(s.vehicles[1].speed>20,"Second car accelerates like the first");
        Input brake;brake.brake=true;run(s,brake,240);
        s.interact();
        check(s.mode==Mode::OnFoot && s.drivenVehicle==-1,"Exiting the second car restores on-foot state");
        check(s.freePosition(s.player.position,0.45f,false),"Second car exit is clear");
        // Eligibility rejects the same way for both instances.
        s.reset();
        s.player.position=s.vehicles[1].position+Vec2{30,0};
        s.interact();
        check(s.mode==Mode::OnFoot,"Far interactable is rejected for the second car too");
        s.player.position=s.vehicles[1].position+Vec2{3,0};
        s.player.height=0.5f;s.interact();
        check(s.mode==Mode::OnFoot,"Airborne player cannot enter the second car");
        // Parked cars are solid to each other: driving into one stops the car.
        s.reset();
        s.vehicles[0].position={2,5};s.vehicles[0].yaw=0;
        s.vehicles[1].position={2,16};
        s.player.position={4.5f,5};s.interact();
        check(s.drivenVehicle==0,"First car still enterable");
        drive={};drive.throttle=1;run(s,drive,300);
        check(s.vehicles[0].position.z<13.0f,"Driven car stops before the parked car");
        check(length(s.vehicles[0].position-s.vehicles[1].position)>=2*Vehicle::Radius-0.05f,
              "Cars never interpenetrate");
        // NPC greeting: proximity + E turns the pedestrian to face the player.
        s.reset();
        s.npcs[0].position={8,-40};s.npcs[0].state=NpcState::Walking;
        s.player.position={9.5f,-40};
        check(s.prompt()=="E  /  Greet pedestrian","Pedestrian offers a greet prompt");
        s.interact();
        check(s.npcs[0].state==NpcState::Idle,"Greeting pauses the pedestrian");
        near(s.npcs[0].yaw,std::atan2(1.5f,0.0f),0.05f,"Pedestrian turns to face the player");
        check(!s.notice.empty(),"Greeting produces a notice");
        std::cout<<"PASS: "<<assertions<<" interaction assertions\n";return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
