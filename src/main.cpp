#include "renderer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

int main(int argc,char** argv) {
    using namespace palm;
    bool smoke=false;
    std::string capture,scene="street";
    for (int i=1;i<argc;++i) {
        std::string arg=argv[i];
        if (arg=="--smoke") smoke=true;
        else if (arg=="--capture" && i+1<argc) capture=argv[++i];
        else if (arg=="--scene" && i+1<argc) scene=argv[++i];
        else { std::cerr<<"Usage: palm_district [--smoke] [--capture path.png] [--scene street|studio|water|drive|welcome]\n"; return 2; }
    }
    if (scene!="street" && scene!="studio" && scene!="water" && scene!="drive" && scene!="welcome") return 2;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | (smoke ? FLAG_WINDOW_HIDDEN : FLAG_VSYNC_HINT));
    InitWindow(1280,800,"Palm District | C++ open-world sandbox v0");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(1000,700);
    SetTextLineSpacing(7);
    SetExitKey(KEY_NULL);
    SetTargetFPS(smoke ? 0 : 120);
    Simulation sim;
    View view;
    view.font=GetFontDefault();
    // Use a local system font when present; no font assets are redistributed.
    const char* candidates[]={"C:/Windows/Fonts/seguisb.ttf","/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf","/System/Library/Fonts/Supplemental/Arial.ttf"};
    bool ownFont=false;
    for (const char* path : candidates) if (FileExists(path)) {
        view.font=LoadFontEx(path,64,nullptr,0); ownFont=true;
        SetTextureFilter(view.font.texture,TEXTURE_FILTER_BILINEAR); break;
    }
    if (smoke) { view.welcome=scene=="welcome"; view.help=false; }
    if (scene=="studio") { sim.player.position=sim.world.entrance;sim.interact();view.yaw=Pi;view.pitch=0.52f; }
    if (scene=="water") {sim.player.position={78,12};sim.step({},FixedStep);view.yaw=Pi/2;view.pitch=0.32f;}
    if (scene=="drive") {sim.player.position={5.2f,5};sim.interact();}
    if (smoke && scene=="street") {view.yaw=3.8f;view.pitch=0.42f;}
    updateCamera(view,sim,FixedStep,true);
    float accumulator=0;
    bool pendingJump=false,pendingInteract=false,quit=false;
    int frames=0;
    while (!WindowShouldClose() && !quit) {
        float dt=smoke ? 1.0f/60.0f : std::min(GetFrameTime(),0.1f);
        if (!smoke && !IsWindowFocused() && !view.welcome) {view.paused=true;EnableCursor();}
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (view.welcome) quit=true;
            else {view.paused=!view.paused; if (view.paused) EnableCursor(); else DisableCursor();}
        }
        if (view.welcome || view.paused) {
            if (IsKeyPressed(KEY_Q)) quit=true;
            if (IsKeyPressed(KEY_ENTER)) {view.welcome=false;view.paused=false;DisableCursor();GetMouseDelta();}
            accumulator=0;pendingJump=false;pendingInteract=false;
        } else {
            if (IsKeyPressed(KEY_H)) view.help=!view.help;
            if (IsKeyPressed(KEY_F3)) view.debug=!view.debug;
            if (IsKeyPressed(KEY_R)) {
                sim.reset();view.yaw=Pi;view.pitch=0.38f;accumulator=0;
                pendingJump=false;pendingInteract=false;updateCamera(view,sim,dt,true);
            }
            if (!smoke) {
                Vector2 mouse=GetMouseDelta();
                view.yaw-=mouse.x*0.003f;
                view.pitch=std::clamp(view.pitch+mouse.y*0.0025f,0.12f,1.12f);
                view.distance=std::clamp(view.distance-GetMouseWheelMove(),5.0f,19.0f);
            }
            Vec2 f=forward(view.yaw),right{-f.z,f.x};
            float longitudinal=static_cast<float>(IsKeyDown(KEY_W)-IsKeyDown(KEY_S));
            float lateral=static_cast<float>(IsKeyDown(KEY_D)-IsKeyDown(KEY_A));
            Input input;
            input.movement=f*longitudinal+right*lateral;
            input.throttle=longitudinal;input.steering=-lateral;
            input.sprint=IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT);
            input.brake=IsKeyDown(KEY_SPACE);
            pendingJump=pendingJump||IsKeyPressed(KEY_SPACE);
            pendingInteract=pendingInteract||IsKeyPressed(KEY_E);
            if (smoke && scene=="drive") input.throttle=1;
            if (smoke && scene=="water") input.movement={0,-0.5f};
            view.moving=length(input.movement)>0.01f;
            accumulator+=dt;
            while (accumulator>=FixedStep) {
                input.jump=pendingJump;input.interact=pendingInteract;
                Mode oldMode=sim.mode;
                sim.step(input,FixedStep);
                if (oldMode!=sim.mode) {
                    if (sim.mode==Mode::Interior) view.yaw=Pi;
                    if (sim.mode==Mode::Driving) view.yaw=sim.car.yaw;
                    updateCamera(view,sim,FixedStep,true);
                }
                pendingJump=false;pendingInteract=false;
                accumulator-=FixedStep;
            }
            updateCamera(view,sim,dt);
        }
        BeginDrawing();
        drawScene(sim,view);drawHud(sim,view);
        EndDrawing();
        ++frames;
        if (smoke && frames==120) {
            if (!capture.empty()) {
                Image screenshot=LoadImageFromScreen();
                bool saved=ExportImage(screenshot,capture.c_str());
                UnloadImage(screenshot);
                if (!saved) {
                    std::cerr<<"Could not save capture: "<<capture<<"\n";
                    if (ownFont) UnloadFont(view.font);
                    CloseWindow();return 1;
                }
            }
            std::cout<<"Render smoke passed: "<<scene<<", "<<frames<<" frames, mode "<<static_cast<int>(sim.mode)<<"\n";
            quit=true;
        }
    }
    EnableCursor();
    if (ownFont) UnloadFont(view.font);
    CloseWindow();
    return 0;
}
