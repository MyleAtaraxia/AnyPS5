#include "prx/libSceVideoOut/include/MessageDialogUI.hpp"
#include "prx/libSceAgcDriver/Execution/include/Presentation.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "SDL.h"
#include "SDL_vulkan.h"
#include <stdexcept>
#include <cstdio>
#include <vector>
extern "C" int APS5_VABI sceCommonDialogInitialize();
int main() {
    if(SDL_InitSubSystem(SDL_INIT_VIDEO)!=0) { std::fprintf(stderr,"SKIP: SDL video: %s\n",SDL_GetError()); return 77; }
    auto* window=SDL_CreateWindow("AnyPS5 message dialog integration",0,0,1280,720,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    if(!window) { std::fprintf(stderr,"SKIP: SDL Vulkan window: %s\n",SDL_GetError()); SDL_QuitSubSystem(SDL_INIT_VIDEO); return 77; }
    try {
        sceCommonDialogInitialize();
        MessageDialogUI ui;
        using MsgDialog::Operation;
        const auto call=[](Operation op,const void* p=nullptr) { return MsgDialogCall_nid_no_patch(op,p,0,0); };
        if(call(Operation::Initialize)) throw std::runtime_error("initialize failed");
        MsgDialog::Param p{}; p.size=sizeof(p); p.base.size=sizeof(p.base); p.mode=1;
        MsgDialog::UserParam u{}; u.msg="Message dialog presentation test"; p.user=&u;
        if(call(Operation::Open,&p)) throw std::runtime_error("open failed");
        unsigned count=0;
        if(!SDL_Vulkan_GetInstanceExtensions(window,&count,nullptr)) throw std::runtime_error(SDL_GetError());
        std::vector<const char*> extensions(count);
        if(!SDL_Vulkan_GetInstanceExtensions(window,&count,extensions.data())) throw std::runtime_error(SDL_GetError());
        const AgcDriver::PresentationWindow target{window,extensions,
            [](void* c,VkInstance instance) {
                VkSurfaceKHR surface{};
                if(!SDL_Vulkan_CreateSurface(static_cast<SDL_Window*>(c),instance,&surface)) throw std::runtime_error(SDL_GetError());
                return surface;
            }, [](void* c,std::uint32_t* w,std::uint32_t* h) {
                int width=0,height=0; SDL_Vulkan_GetDrawableSize(static_cast<SDL_Window*>(c),&width,&height);
                *w=width; *h=height;
            },1280,720,{}};
        bool ready=false;
        AgcDriverPresentDialog_nid_postfix(target,ui.Draw(),[](void* c) { *static_cast<bool*>(c)=true; },&ready);
        if(!ready) throw std::runtime_error("GPU callback missing");
        ui.Presented();
        SDL_Event event{}; event.type=SDL_KEYDOWN; event.key.windowID=SDL_GetWindowID(window); event.key.keysym.sym=SDLK_RETURN;
        ui.HandleEvent(event,event.key.windowID);
        MsgDialogResult result{};
        if(call(Operation::GetResult,&result) || result.button_id!=1) throw std::runtime_error("selection not returned");
        call(Operation::Terminate);
        AgcDriverReleaseWindow_nid_postfix(window);
        AgcDriverShutdown_nid_postfix();
        SDL_DestroyWindow(window); SDL_QuitSubSystem(SDL_INIT_VIDEO);
        LibcRunShutdown_nid_postfix(); return 0;
    } catch(const std::exception& error) { std::fprintf(stderr,"%s\n",error.what()); return 1; }
}
