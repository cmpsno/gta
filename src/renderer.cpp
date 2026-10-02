#include "renderer.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace palm {
namespace {
const Color Ink{22,40,45,255}, Mint{164,233,199,255}, Paper{247,241,222,255};
const Color Asphalt{65,79,80,255}, Sand{216,200,159,255}, Ocean{52,146,151,255};
const Color Facades[]={{217,163,139,255},{225,199,157,255},{145,173,165,255},{175,178,191,255}};
Color shade(Color c,float v) { return {static_cast<unsigned char>(c.r*v),static_cast<unsigned char>(c.g*v),static_cast<unsigned char>(c.b*v),c.a}; }
void cube(Vector3 p, float w,float h,float d,Color color) {
    // Face colors give the primitives a consistent, inexpensive sun direction.
    float x=p.x-w/2, y=p.y-h/2, z=p.z-d/2;
    float X=x+w,Y=y+h,Z=z+d;
    rlBegin(RL_QUADS);
    auto face=[&](Color c,Vector3 a,Vector3 b,Vector3 e,Vector3 f) {
        rlColor4ub(c.r,c.g,c.b,c.a);
        for (Vector3 v : {a,b,e,f}) rlVertex3f(v.x,v.y,v.z);
    };
    face(shade(color,0.81f),{x,y,Z},{X,y,Z},{X,Y,Z},{x,Y,Z});
    face(shade(color,0.89f),{X,y,z},{x,y,z},{x,Y,z},{X,Y,z});
    face(shade(color,0.69f),{x,y,z},{x,y,Z},{x,Y,Z},{x,Y,z});
    face(shade(color,0.94f),{X,y,Z},{X,y,z},{X,Y,z},{X,Y,Z});
    face(color,{x,Y,Z},{X,Y,Z},{X,Y,z},{x,Y,z});
    face(shade(color,0.6f),{x,y,z},{X,y,z},{X,y,Z},{x,y,Z});
    rlEnd();
}
void text(const View& v,const std::string& s,float x,float y,float size,Color c) {
    DrawTextEx(v.font,s.c_str(),{x,y},size,0.6f,c);
}
void panel(Rectangle r,Color c) { DrawRectangleRounded(r,0.12f,6,c); }
void palmTree(float x,float z,float height,float sway) {
    DrawCylinderEx({x,0,z},{x+0.5f,height,z},0.26f,0.19f,6,{125,113,86,255});
    Vector3 top{x+0.5f,height,z};
    for (int k=0;k<7;++k) {
        float a=k*2*Pi/7+sway;
        Vector3 tip{x+std::sin(a)*3.8f,height-0.6f,z+std::cos(a)*3.8f};
        Vector3 side{std::cos(a)*0.65f,0,-std::sin(a)*0.65f};
        Vector3 mid{x+std::sin(a)*1.8f,height+0.4f,z+std::cos(a)*1.8f};
        Color leaf=k%2 ? Color{70,114,96,255} : Color{90,139,106,255};
        DrawTriangle3D(top,Vector3Add(mid,side),tip,leaf);
        DrawTriangle3D(top,tip,Vector3Subtract(mid,side),leaf);
        DrawTriangle3D(tip,Vector3Add(mid,side),top,leaf);
        DrawTriangle3D(Vector3Subtract(mid,side),tip,top,leaf);
    }
}
void carModel(const Vehicle& c,float time,bool occupied) {
    DrawCylinder({c.position.x,0.025f,c.position.z},2.7f,2.7f,0.01f,24,{48,62,59,90});
    rlPushMatrix();
    rlTranslatef(c.position.x,0,c.position.z);
    rlRotatef(c.yaw*RAD2DEG,0,1,0);
    cube({0,0.78f,0},2.05f,0.65f,4.1f,{221,125,95,255});
    cube({0,1.31f,-0.3f},1.75f,0.62f,1.94f,{204,108,82,255});
    cube({0,1.38f,0.69f},1.55f,0.40f,0.025f,{55,89,96,255});
    cube({0,1.38f,-1.28f},1.55f,0.36f,0.025f,{55,89,96,255});
    for (float x : {-0.887f,0.887f}) {
        cube({x,1.38f,-0.3f},0.018f,0.4f,1.54f,{55,89,96,255});
        cube({x,1.38f,-0.3f},0.03f,0.45f,0.10f,{204,108,82,255});
    }
    for (float x : {-1.03f,1.03f}) for (float z : {-1.3f,1.3f}) {
        cube({x,0.47f,z},0.28f,0.72f,0.73f,{29,40,43,255});
        cube({x*1.15f,0.47f,z},0.015f,0.33f,0.34f,{180,182,167,255});
    }
    for (float x : {-0.65f,0.65f}) {
        cube({x,0.85f,2.06f},0.5f,0.22f,0.025f,{255,232,170,255});
        cube({x,0.85f,-2.06f},0.48f,0.2f,0.025f,{173,67,56,255});
    }
    cube({0,0.5f,2.08f},1.84f,0.15f,0.10f,{168,178,168,255});
    rlPopMatrix();
    if (!occupied) {
        float y=3.1f+std::sin(time*2)*0.15f;
        DrawSphere({c.position.x,y,c.position.z},0.17f,Mint);
        DrawCylinderWires({c.position.x,0.045f,c.position.z},3.25f,3.25f,0.015f,36,Mint);
    }
}
void playerModel(const Player& p,float time,bool moving) {
    float y=p.position.y+(p.swimming ? -0.7f : 0.0f);
    if (!p.swimming) DrawCylinder({p.position.x,0.03f,p.position.z},0.52f,0.52f,0.01f,16,{48,62,59,100});
    rlPushMatrix();
    rlTranslatef(p.position.x,y,p.position.z);
    rlRotatef(p.yaw*RAD2DEG,0,1,0);
    float stride=moving ? std::sin(time*12)*0.28f : 0;
    cube({0,1.12f,0},0.66f,0.66f,0.39f,Mint);
    cube({0,1.72f,0},0.43f,0.43f,0.43f,{193,144,109,255});
    cube({0,1.94f,-0.02f},0.45f,0.12f,0.45f,Ink);
    for (float side : {-1.0f,1.0f}) {
        cube({side*0.19f,0.41f,side*stride},0.23f,0.74f,0.28f,{50,67,78,255});
        cube({side*0.19f,0.09f,0.06f+side*stride},0.27f,0.16f,0.42f,Paper);
        cube({side*0.44f,1.06f,-side*stride},0.19f,0.69f,0.24f,{193,144,109,255});
    }
    rlPopMatrix();
}
void npcModel(const Npc& n,float time) {
    // V0.2: same primitive visual language as the player, terracotta so the
    // pedestrian reads as a different actor at a glance.
    rlPushMatrix();
    rlTranslatef(n.position.x,0,n.position.z);
    rlRotatef(n.yaw*RAD2DEG,0,1,0);
    float bob=n.state==NpcState::Walking ? std::abs(std::sin(time*9))*0.06f : 0;
    cube({0,0.95f+bob,0},0.62f,0.95f,0.36f,{196,118,92,255});
    cube({0,1.68f+bob,0},0.40f,0.40f,0.40f,{193,144,109,255});
    cube({0,1.90f+bob,-0.02f},0.42f,0.12f,0.42f,Ink);
    for (float side : {-1.0f,1.0f})
        cube({side*0.18f,0.30f,0},0.22f,0.60f,0.26f,{58,72,84,255});
    rlPopMatrix();
}
void exterior(const Simulation& s,const View& v) {
    cube({0,-0.32f,0},208,0.6f,208,Sand);
    cube({-16,-0.02f,0},176,0.06f,208,{173,185,155,255});
    cube({70,0.005f,0},8,0.07f,208,Sand);
    cube({127,-0.05f,0},106,0.07f,420,Ocean);
    for (float x : {-56.0f,0.0f,56.0f}) {
        cube({x,0.015f,0},17,0.08f,208,{195,194,174,255});
        cube({x,0.063f,0},12,0.025f,208,Asphalt);
    }
    for (float z : {-56.0f,0.0f,56.0f}) {
        cube({-20,0.04f,z},164,0.06f,17,{195,194,174,255});
        cube({-20,0.081f,z},164,0.025f,12,Asphalt);
    }
    for (int i=-100;i<=100;i+=8) {
        bool intersection=std::abs(i)<10 || std::abs(i-56)<10 || std::abs(i+56)<10;
        if (!intersection) for (float x : {-56.0f,0.0f,56.0f})
            cube({x,0.105f,static_cast<float>(i)},0.14f,0.01f,3.5f,{217,198,139,255});
        if (i<64 && !intersection) for (float z : {-56.0f,0.0f,56.0f})
            cube({static_cast<float>(i),0.105f,z},3.5f,0.01f,0.14f,{217,198,139,255});
    }
    for (float x : {-56.0f,0.0f,56.0f}) for (float z : {-56.0f,0.0f,56.0f})
        for (int stripe=-4;stripe<=4;stripe+=2) for (float side : {-8.0f,8.0f}) {
            cube({x+stripe,0.105f,z+side},1,0.015f,2.6f,Paper);
            cube({x+side,0.105f,z+stripe},2.6f,0.015f,1,Paper);
        }
    for (const Block& b : s.world.buildings) {
        const AABB& bb=b.bounds;
        float cx=(bb.min.x+bb.max.x)/2, cz=(bb.min.z+bb.max.z)/2;
        float w=bb.max.x-bb.min.x, h=bb.max.y-bb.min.y, d=bb.max.z-bb.min.z;
        if (Vector3Distance(v.camera.position,{cx,0,cz})>155) continue;
        cube({cx-2,0.115f,cz+2},w+4,0.01f,d+4,{115,137,121,255});
        Color c=Facades[b.palette];
        cube({cx,bb.min.y+h/2,cz},w,h,d,c);
        cube({cx,bb.max.y+0.18f,cz},w+0.45f,0.36f,d+0.45f,Paper);
        cube({cx+2,bb.max.y+0.8f,cz-2},4,1.3f,4,shade(c,0.8f));
        for (float y=bb.min.y+3;y<bb.max.y-1;y+=3.8f) {
            for (float x=bb.min.x+2.7f;x<bb.max.x-1;x+=4.3f) {
                cube({x,y,bb.max.z+0.035f},1.6f,1.8f,0.06f,{65,95,99,255});
                cube({x,y,bb.min.z-0.035f},1.6f,1.8f,0.06f,{78,109,112,255});
                cube({x,y-0.95f,bb.max.z+0.10f},1.9f,0.12f,0.25f,Paper);
            }
            for (float z=bb.min.z+3;z<bb.max.z-1;z+=4.3f) {
                cube({bb.max.x+0.035f,y,z},0.06f,1.8f,1.6f,{65,95,99,255});
                cube({bb.min.x-0.035f,y,z},0.06f,1.8f,1.6f,{65,95,99,255});
            }
        }
    }
    // Phase 1 task 5: test staircase. floors[0] is the ground slab, already
    // drawn as sand; every later slab renders as a concrete step.
    for (std::size_t i=1;i<s.world.floors.size();++i) {
        const AABB& f=s.world.floors[i];
        cube({(f.min.x+f.max.x)/2,(f.min.y+f.max.y)/2,(f.min.z+f.max.z)/2},
             f.max.x-f.min.x,f.max.y-f.min.y,f.max.z-f.min.z,{148,168,158,255});
    }
    // Studio landmark. Its door is a transition trigger, not a gap in collision.
    cube({26,1.55f,-10.94f},2.4f,3.1f,0.12f,Mint);    cube({26,3.4f,-10.4f},6,0.3f,2.3f,{71,107,96,255});
    DrawCylinderWires({26,0.15f,-9.6f},2,2,0.02f,32,Mint);
    for (int i=-90;i<=90;i+=18) {
        palmTree(68,static_cast<float>(i),6.5f+(i%3),0.4f);
        if (i%36==0) palmTree(-9,static_cast<float>(i),6,0);
    }
    // Surface ripples move without allocating meshes or textures each frame.
    for (int i=0;i<20;++i) for (int j=0;j<8;++j) {
        float x=78+j*9.0f+std::sin(s.time*0.65f+i)*0.8f;
        float z=-100+i*10.5f+std::sin(j*2.0f)*3;
        cube({x,0.003f,z},3.2f,0.015f,0.13f,{108,183,179,255});
    }
    // Shoreline reads as a continuous, walkable transition into shallow water.
    cube({74.2f,0.012f,0},0.4f,0.025f,208,{205,227,201,255});
    // V0.3: every registered vehicle renders; the driven one hides its marker.
    const Vehicle* occupied=s.driven();
    for (const auto& veh : s.vehicles) carModel(veh,s.time,&veh==occupied);
    for (const auto& n : s.npcs) npcModel(n,s.time);
}
void interior(const Simulation& s) {
    cube({0,-0.15f,0},21,0.3f,21,{200,180,146,255});
    for (int i=-9;i<10;i+=2) cube({static_cast<float>(i),0.005f,0},0.035f,0.01f,19,{169,149,121,255});
    for (std::size_t i=0;i<s.world.room.size();++i) {
        const Block& b=s.world.room[i];
        const AABB& bb=b.bounds;
        cube({(bb.min.x+bb.max.x)/2,(bb.min.y+bb.max.y)/2,(bb.min.z+bb.max.z)/2},
             bb.max.x-bb.min.x,bb.max.y-bb.min.y,bb.max.z-bb.min.z,
             i<4 ? Paper : Facades[b.palette]);
    }
    cube({0,1.55f,9.45f},2.8f,3.1f,0.06f,Mint);
    cube({-9.44f,2.7f,-2},0.06f,2.2f,5,{116,176,174,255});
    cube({0,0.035f,0},7,0.06f,6,{109,147,134,255});
    cube({-5,1.85f,-5},1.2f,0.85f,0.1f,Ink);
    cube({5,1.35f,-6.3f},2.7f,0.7f,0.9f,{231,194,144,255});
    cube({-9.42f,3.3f,4},0.05f,2,1.7f,Facades[0]);
    DrawCylinderWires({0,0.08f,7.8f},1.5f,1.5f,0.02f,32,Mint);
}
void minimap(const Simulation& s,const View& v,float x,float y,float size) {
    panel({x-10,y-10,size+20,size+42},Fade(Ink,0.93f));
    DrawRectangle(static_cast<int>(x),static_cast<int>(y),static_cast<int>(size),static_cast<int>(size),{129,152,133,255});
    auto at=[&](Vec2 p) { return Vector2{x+(p.x+104)/208*size,y+(p.z+104)/208*size}; };
    float scale=size/208;
    Vector2 shore=at({74,-104});
    DrawRectangleV(shore,{30*scale,size},Ocean);
    for (float road : {-56.0f,0.0f,56.0f}) {
        Vector2 a=at({road-6,-104}); DrawRectangleV(a,{12*scale,size},Asphalt);
        a=at({-104,road-6}); DrawRectangleV(a,{178*scale,12*scale},Asphalt);
    }
    for (const Block& b : s.world.buildings) {
        Vector2 a=at({b.bounds.min.x,b.bounds.min.z});
        DrawRectangleV(a,{(b.bounds.max.x-b.bounds.min.x)*scale,(b.bounds.max.z-b.bounds.min.z)*scale},{201,195,169,255});
    }
    Vector2 door=at(s.world.entrance); DrawCircleV(door,3.5f,Mint);
    for (const auto& veh : s.vehicles) {
        Vector2 dot=at(planar(veh.position)); DrawRectangleV({dot.x-3,dot.y-3},{6,6},{234,137,103,255});
    }
    for (const auto& n : s.npcs) { Vector2 d=at(n.position); DrawCircleV(d,2.5f,{196,118,92,255}); }
    Vec2 pos=s.mode==Mode::Interior ? s.world.entrance : s.focus();
    Vector2 p=at(pos); Vec2 f=forward(s.mode==Mode::Driving && s.driven() ? s.driven()->yaw : s.player.yaw);
    Vector2 tip{p.x+f.x*7,p.y+f.z*7},left{p.x-f.x*4+f.z*4,p.y-f.z*4-f.x*4},right{p.x-f.x*4-f.z*4,p.y-f.z*4+f.x*4};
    DrawTriangle(tip,left,right,Paper); DrawCircleV(p,2,Paper);
    text(v,"N",x+size-13,y+4,15,Paper);
    text(v,"PALM DISTRICT",x,y+size+9,14,Mint);
}
}
void updateCamera(View& v,const Simulation& s,float dt,bool snap) {
    Vec2 p=s.focus();
    float targetY=s.mode==Mode::Driving ? 1.4f : (s.player.swimming ? 0.7f : 1.3f+s.player.position.y*0.5f);
    Vector3 target{p.x,targetY,p.z};
    float distance=s.mode==Mode::Interior ? std::min(v.distance,7.0f) : v.distance+(s.mode==Mode::Driving ? 3.0f : 0);
    Vector3 offset{-std::sin(v.yaw)*std::cos(v.pitch)*distance,std::sin(v.pitch)*distance,-std::cos(v.yaw)*std::cos(v.pitch)*distance};
    Vector3 desired=Vector3Add(target,offset);
    Ray ray{target,Vector3Normalize(offset)};
    float nearest=distance;
    for (const auto& b : s.mode==Mode::Interior ? s.world.room : s.world.buildings) {
        BoundingBox box{{b.bounds.min.x-0.2f,-0.1f,b.bounds.min.z-0.2f},
                        {b.bounds.max.x+0.2f,b.bounds.max.y+0.2f,b.bounds.max.z+0.2f}};
        RayCollision hit=GetRayCollisionBox(ray,box);
        if (hit.hit && hit.distance<nearest) nearest=std::max(0.4f,hit.distance-0.3f);
    }
    desired=Vector3Add(target,Vector3Scale(ray.direction,nearest));
    // V0.1 feel: a one-frame landing flag becomes a short shake; sprint and
    // speed widen the FOV slightly. Both decay/are derived, never stored in sim.
    if (s.justLanded) v.shake=1.0f;
    v.shake*=std::exp(-7.0f*dt);
    if (v.shake>0.003f) {
        desired.x+=std::sin(s.time*91.0f)*0.14f*v.shake;
        desired.y+=std::abs(std::sin(s.time*73.0f))*0.11f*v.shake;
    }
    // Collision correction is immediate; only target following is smoothed.
    v.camera.target = snap ? target : Vector3Lerp(v.camera.target,target,1-std::exp(-14*dt));
    v.camera.position=desired;
    v.camera.up={0,1,0};
    const Vehicle* moving=s.driven();
    float speedKmh=moving ? std::abs(moving->speed) : 0;
    v.camera.fovy=s.mode==Mode::Driving ? 65.0f+std::min(9.0f,speedKmh*0.28f)
                                        : 58.0f+(s.sprinting ? 4.0f : 0.0f);
    v.camera.projection=CAMERA_PERSPECTIVE;
}
void drawScene(const Simulation& s,const View& v) {
    ClearBackground({213,223,210,255});
    DrawRectangleGradientV(0,0,GetScreenWidth(),GetScreenHeight(),{181,207,205,255},{246,224,187,255});
    BeginMode3D(v.camera);
    if (s.mode==Mode::Interior) interior(s); else exterior(s,v);
    if (s.mode!=Mode::Driving) playerModel(s.player,s.time,v.moving);
    if (v.debug) {
        for (const auto& b : s.mode==Mode::Interior ? s.world.room : s.world.buildings) {
            const AABB& bb=b.bounds;
            DrawCubeWires({(bb.min.x+bb.max.x)/2,(bb.min.y+bb.max.y)/2,(bb.min.z+bb.max.z)/2},
                          bb.max.x-bb.min.x+0.04f,bb.max.y-bb.min.y+0.04f,bb.max.z-bb.min.z+0.04f,MAGENTA);
        }
        Vec2 p=s.focus(); float radius=s.mode==Mode::Driving ? Vehicle::Radius : 0.45f;
        DrawCylinderWires({p.x,0.1f,p.z},radius,radius,2,24,MAGENTA);
    }
    EndMode3D();
}
void drawHud(const Simulation& s,const View& v) {
    float w=static_cast<float>(GetScreenWidth()),h=static_cast<float>(GetScreenHeight());
    text(v,"PALM DISTRICT",30,24,32,Ink);
    text(v,s.mode==Mode::Interior ? "THE STUDIO  /  INTERIOR" : "COASTAL SANDBOX  /  FREE ROAM",32,61,14,Ink);
    panel({w-195,25,165,36},Fade(Ink,0.93f));
    text(v,"V0.4  /  C++",w-176,34,17,Mint);
    text(v,std::to_string(GetFPS())+" FPS",w-95,72,14,Ink);
    minimap(s,v,40,h-235,170);
    const char* label=s.mode==Mode::Driving ? "DRIVING" : s.mode==Mode::Interior ? "IN THE STUDIO" : s.player.swimming ? "SWIMMING" : "ON FOOT";
    panel({w-265,h-233,235,203},Fade(Ink,0.94f));
    text(v,label,w-246,h-215,15,Mint);
    if (s.mode==Mode::Driving) {
        const Vehicle* d=s.driven();
        text(v,std::to_string(static_cast<int>(std::abs(d ? d->speed : 0)*3.6f)),w-247,h-189,46,Paper);
        text(v,"KM/H",w-158,h-164,14,Paper);
    } else {
        text(v,std::to_string(s.progress.completed())+" / 6",w-246,h-187,38,Paper);
        text(v,"MECHANICS EXPLORED",w-246,h-140,13,Paper);
    }
    const bool done[]={s.progress.walked>=20,s.progress.jumped,s.progress.enteredCar,s.progress.driven>=80,s.progress.visitedRoom,s.progress.swam};
    for (int i=0;i<6;++i) DrawRectangleRounded({w-245+i*32,h-106,24,5},0.5f,4,done[i] ? Mint : Color{76,96,91,255});
    text(v,"H  Help     F3  Collisions",w-246,h-82,14,Paper);
    text(v,"R  Reset    Esc  Pause",w-246,h-58,14,Paper);
    std::string message=s.noticeTime>0 ? s.notice : s.prompt();
    float fontSize=17;
    float tw=MeasureTextEx(v.font,message.c_str(),fontSize,0.6f).x;
    float available=std::max(200.0f,w-580);
    if (tw>available) {fontSize*=available/tw;tw=available;}
    panel({(w-tw)/2-18,h-81,tw+36,44},Fade(Ink,0.93f));
    text(v,message,(w-tw)/2,h-68,fontSize,Paper);
    if (v.help && !v.welcome) {
        panel({30,102,274,205},Fade(Ink,0.92f));
        text(v,"MAKE YOURSELF AT HOME",47,119,16,Mint);
        text(v,"WASD     Move / drive\nMouse    Look around\nShift       Sprint\nSpace     Jump / brake\nE             Enter / exit\nWheel     Camera distance",47,150,17,Paper);
    }
    if (v.debug) {
        Vec2 p=s.focus();
        text(v,TextFormat("x %.1f   z %.1f   height %.2f   collision %s",p.x,p.z,s.player.position.y,s.collided?"yes":"no"),w/2-210,28,17,Ink);
        // V0.4: frame-time and object-count instrumentation on the F3 overlay.
        text(v,TextFormat("frame %.2f ms   sim %.2f ms   render %.2f ms",v.simMs+v.renderMs,v.simMs,v.renderMs),w/2-210,52,17,Ink);
        text(v,TextFormat("npcs %d   vehicles %d   box checks/step %ld",static_cast<int>(s.npcs.size()),static_cast<int>(s.vehicles.size()),s.collisionChecks),w/2-210,76,17,Ink);
    }
    if (v.paused || v.welcome) {
        DrawRectangle(0,0,static_cast<int>(w),static_cast<int>(h),Fade(Ink,0.48f));
        float cx=w/2-260,cy=h/2-214;
        panel({cx,cy,520,428},Ink);
        text(v,"PALM DISTRICT     /     V0",cx+38,cy+31,17,Mint);
        text(v,v.welcome ? "A city. A car.\nYour next move." : "Take a breather.",cx+36,cy+80,v.welcome ? 42.0f : 37.0f,Paper);
        text(v,v.welcome ? "Walk the blocks. Borrow the car.\nStep inside the studio. Find the water." : "Your sandbox is paused.\nCome back whenever you're ready.",cx+38,cy+199,20,Paper);
        panel({cx+38,cy+284,444,53},Mint);
        text(v,v.welcome ? "ENTER  /  EXPLORE THE DISTRICT" : "ENTER  /  BACK TO THE DISTRICT",cx+58,cy+300,19,Ink);
        text(v,"WASD move   /   Mouse look   /   H help",cx+38,cy+354,16,Paper);
        text(v,"Q quit   /   Procedural shapes. Six mechanics.",cx+38,cy+381,14,{158,182,168,255});
    }
}
} // namespace palm
