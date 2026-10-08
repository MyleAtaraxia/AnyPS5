#include "prx/libSceVideoOut/include/MessageDialogUI.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "SDL.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(expr) do { if(!(expr)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#expr); std::abort(); } } while(0)
extern "C" int APS5_VABI sceCommonDialogInitialize();
using namespace MsgDialog;
int Call(Operation op,const void* p=nullptr) { return MsgDialogCall_nid_no_patch(op,p,0,0); }
int main() {
    CHECK(sceCommonDialogInitialize()==0);
    MessageDialogUI ui;
    CHECK(Call(Operation::Initialize)==0);
    MsgDialog::Param p{}; p.base.size=sizeof(p.base); p.size=sizeof(p); p.mode=1;
    UserParam u{}; u.buttonType=1; u.msg="Choose an option: café, 日本語\nSecond line"; p.user=&u;
    CHECK(Call(Operation::Open,&p)==0);
    auto pixels=ui.Draw(); CHECK(pixels.size()==1280*720*4);
    ui.Presented();
    const auto checksum=[](std::span<const std::byte> frame) {
        std::uint64_t value=1469598103934665603ull;
        for(const auto byte:frame) value=(value^std::to_integer<unsigned>(byte))*1099511628211ull;
        return value;
    };
    const auto firstFrame=checksum(pixels);
    if(const char* path=std::getenv("ANYPS5_MSG_DIALOG_SCREENSHOT")) {
        auto* surface=SDL_CreateRGBSurfaceWithFormatFrom(const_cast<std::byte*>(pixels.data()),1280,720,32,1280*4,SDL_PIXELFORMAT_BGRA32);
        CHECK(surface && SDL_SaveBMP(surface,path)==0); SDL_FreeSurface(surface);
    }
    SDL_Event event{}; event.type=SDL_KEYDOWN; event.key.windowID=42; event.key.keysym.sym=SDLK_LEFT;
    CHECK(!ui.HandleEvent(event,43));
    CHECK(ui.HandleEvent(event,42));
    View v; CHECK(MsgDialogView_nid_no_patch(&v) && v.selected==1);
    CHECK(checksum(ui.Draw())!=firstFrame);
    event.key.keysym.sym=SDLK_RETURN; event.key.repeat=1;
    CHECK(ui.HandleEvent(event,42)); CHECK(Call(Operation::GetStatus)==2);
    event.key.repeat=0; CHECK(ui.HandleEvent(event,42));
    MsgDialogResult r; CHECK(Call(Operation::GetResult,&r)==0 && r.button_id==2);
    CHECK(ui.Draw().empty());
    u.buttonType=3; CHECK(Call(Operation::Open,&p)==0); CHECK(!ui.Draw().empty()); ui.Presented();
    event.key.keysym.sym=SDLK_ESCAPE; CHECK(ui.HandleEvent(event,42));
    CHECK(Call(Operation::GetResult,&r)==0 && r.result==1 && r.button_id==0);
    CHECK(Call(Operation::Open,&p)==0); ui.Draw(); ui.Presented();
    event={}; event.type=SDL_CONTROLLERAXISMOTION; event.caxis.axis=SDL_CONTROLLER_AXIS_LEFTX; event.caxis.value=24000;
    CHECK(ui.HandleEvent(event,42)); CHECK(ui.HandleEvent(event,42));
    CHECK(MsgDialogView_nid_no_patch(&v) && v.selected==1);
    event={}; event.type=SDL_CONTROLLERBUTTONDOWN; event.cbutton.button=SDL_CONTROLLER_BUTTON_A;
    CHECK(ui.HandleEvent(event,42)); CHECK(Call(Operation::GetResult,&r)==0 && r.result==1);
    u.buttonType=0; CHECK(Call(Operation::Open,&p)==0); ui.Draw(); ui.Presented();
    event.cbutton.button=SDL_CONTROLLER_BUTTON_B; CHECK(ui.HandleEvent(event,42)); CHECK(Call(Operation::GetStatus)==2);
    event.cbutton.button=SDL_CONTROLLER_BUTTON_A; CHECK(ui.HandleEvent(event,42)); CHECK(Call(Operation::GetStatus)==3);
    std::string longText(4000,'x'); u.msg=longText.c_str(); CHECK(Call(Operation::Open,&p)==0); ui.Draw(); ui.Presented();
    event={}; event.type=SDL_KEYDOWN; event.key.windowID=42; event.key.keysym.sym=SDLK_DOWN;
    CHECK(ui.HandleEvent(event,42)); CHECK(MsgDialogView_nid_no_patch(&v) && v.scroll==1);
    for(int i=0;i<1000;++i) CHECK(ui.HandleEvent(event,42));
    CHECK(MsgDialogView_nid_no_patch(&v) && v.scroll==v.maxScroll);
    CHECK(!ui.Draw().empty()); CHECK(Call(Operation::Close)==0);
    CHECK(ui.Draw().empty()); CHECK(Call(Operation::Terminate)==0);
}
