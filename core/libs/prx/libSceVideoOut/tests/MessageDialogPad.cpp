#include "prx/libSceVideoOut/include/PadInput.hpp"
#include "prx/libSceVideoOut/include/DisplayWindow.hpp"
#include "SDL.h"
#include <cstdlib>
#include <cstdio>
#define CHECK(expr) do { if(!(expr)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#expr); std::abort(); } } while(0)
static PadInputState published;
extern "C" void PadPublishInput_nid_postfix(const PadInputState& state) { published=state; }
extern "C" bool PadFetchOutput_nid_postfix(std::uint32_t*,PadOutputState*) { return false; }
std::vector<Pad::InputBinding> Pad::LoadInputMapping() { return {InputMapping.begin(),InputMapping.end()}; }
DisplayWindow::~DisplayWindow()=default;
SDL_Window* DisplayWindow::Handle() const { return nullptr; }
void DisplayWindow::ToggleFullscreen() {}
int main() {
    CHECK(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0);
    const int index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
    CHECK(index>=0 && SDL_IsGameController(index));
    auto* joystick=SDL_JoystickOpen(index); CHECK(joystick);
    {
        PadInput input; DisplayWindow window;
        constexpr auto cross=static_cast<std::uint32_t>(Pad::PadButton::Cross);
        CHECK(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1)==0);
        input.Update(); CHECK(published.buttons&cross);
        input.SetDialogActive(true); CHECK(published.buttons==0);
        input.Update(); CHECK(published.buttons==0);
        input.SetDialogActive(false); CHECK(published.buttons==0);
        SDL_Event e{}; e.type=SDL_CONTROLLERBUTTONDOWN; e.cbutton.button=SDL_CONTROLLER_BUTTON_A;
        input.HandleEvent(e,window); CHECK(published.buttons==0);
        input.Update(); CHECK(published.buttons==0);
        CHECK(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0)==0);
        input.Update(); CHECK(published.buttons==0);
        CHECK(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1)==0);
        input.Update(); CHECK(published.buttons&cross);
        CHECK(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0)==0);
        input.Update();
        input.SetDialogActive(true);
        e={}; e.type=SDL_KEYDOWN; e.key.keysym.scancode=SDL_SCANCODE_RETURN;
        input.HandleEvent(e,window); CHECK(published.buttons==0);
        input.SetDialogActive(false); input.Update(); CHECK(published.buttons==0);
        e.type=SDL_KEYUP; input.HandleEvent(e,window); input.Update(); CHECK(published.buttons==0);
        e.type=SDL_KEYDOWN; input.HandleEvent(e,window); CHECK(published.buttons&cross);
    }
    SDL_JoystickClose(joystick); CHECK(SDL_JoystickDetachVirtual(index)==0);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}
