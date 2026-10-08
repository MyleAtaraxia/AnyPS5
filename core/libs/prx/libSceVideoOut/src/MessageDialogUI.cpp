#include "prx/libSceVideoOut/include/MessageDialogUI.hpp"
#include "SDL.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <cstdio>

struct MessageDialogUI::Font {
    FT_Library library{}; FT_Face face{};
    Font() {
        if (FT_Init_FreeType(&library)) return;
        std::vector<std::filesystem::path> paths;
        if (const char* dir=std::getenv("ANYPS5_SYSTEM_FONTS")) {
            paths.emplace_back(std::filesystem::path(dir)/"NotoSans-Regular.ttf");
            paths.emplace_back(std::filesystem::path(dir)/"NotoSans-Light.ttf");
        }
        if (char* base=SDL_GetBasePath()) {
            paths.emplace_back(std::filesystem::path(base)/"anyps5-fonts"/"NotoSans-Regular.ttf");
            SDL_free(base);
        }
#ifdef _WIN32
        if (const char* windows=std::getenv("WINDIR")) paths.emplace_back(std::filesystem::path(windows)/"Fonts"/"segoeui.ttf");
#elif defined(__APPLE__)
        paths.emplace_back("/System/Library/Fonts/Supplemental/Arial.ttf");
#else
        paths.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
        paths.emplace_back("/usr/share/fonts/truetype/open-sans/OpenSans-Regular.ttf");
#endif
        for (const auto& path:paths) if (!FT_New_Face(library,path.string().c_str(),0,&face)) break;
        if (face) FT_Set_Pixel_Sizes(face,0,26);
    }
    ~Font() { if(face) FT_Done_Face(face); if(library) FT_Done_FreeType(library); }
};
MessageDialogUI::MessageDialogUI():font(std::make_unique<Font>()),pixels(1280*720*4) {
    if (!font->face) std::fprintf(stderr,"[msgdialog] No UI font; install NotoSans-Regular.ttf in anyps5-fonts (or ANYPS5_SYSTEM_FONTS). Dialog open will return NOT_SUPPORTED.\n");
    MsgDialogRenderer_nid_no_patch(font->face!=nullptr);
}
MessageDialogUI::~MessageDialogUI() { MsgDialogRenderer_nid_no_patch(false); }
void MessageDialogUI::rect(int x,int y,int w,int h,unsigned color) {
    for(int row=std::max(0,y);row<std::min(720,y+h);++row)
        for(int col=std::max(0,x);col<std::min(1280,x+w);++col) {
            auto* p=pixels.data()+(row*1280+col)*4;
            p[0]=std::byte(color&255); p[1]=std::byte((color>>8)&255); p[2]=std::byte((color>>16)&255); p[3]=std::byte{255};
        }
}
namespace {
unsigned Codepoint(const std::string& s, std::size_t& i) {
    unsigned c=static_cast<unsigned char>(s[i++]);
    if(c<128) return c;
    int n=(c&0xe0)==0xc0?1:(c&0xf0)==0xe0?2:(c&0xf8)==0xf0?3:0;
    if(!n) return 0xfffd;
    unsigned cp=c&((1u<<(6-n))-1);
    for(int k=0;k<n;++k) {
        if(i>=s.size() || (static_cast<unsigned char>(s[i])&0xc0)!=0x80) return 0xfffd;
        cp=(cp<<6)|(static_cast<unsigned char>(s[i++])&63);
    }
    if(cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff) || cp<(n==1?0x80u:n==2?0x800u:0x10000u)) return 0xfffd;
    return cp;
}
int Lines(FT_Face face, const std::string& s,int width) {
    int lines=1,pen=0;
    for(std::size_t i=0;i<s.size();) {
        const auto c=Codepoint(s,i);
        if(c=='\n') { ++lines; pen=0; continue; }
        if(FT_Load_Char(face,c,FT_LOAD_DEFAULT)) continue;
        const int advance=static_cast<int>(face->glyph->advance.x>>6);
        if(pen+advance>width) { ++lines; pen=0; }
        pen+=advance;
    }
    return lines;
}
}
void MessageDialogUI::text(const std::string& s,int x,int y,int width,int bottom,int clipTop) {
    int pen=x, baseline=y+26;
    for(std::size_t i=0;i<s.size();) {
        const auto c=Codepoint(s,i);
        if(c=='\n') { pen=x; baseline+=36; continue; }
        if(FT_Load_Char(font->face,c,FT_LOAD_RENDER)) continue;
        const auto& g=*font->face->glyph;
        const int advance=static_cast<int>(g.advance.x>>6);
        if(pen+advance>x+width) { pen=x; baseline+=36; }
        if(baseline-26>=bottom) break;
        for(unsigned row=0;row<g.bitmap.rows;++row) for(unsigned col=0;col<g.bitmap.width;++col) {
            const int px=pen+g.bitmap_left+col, py=baseline-g.bitmap_top+row;
            if(px<0 || px>=1280 || py<std::max(y,clipTop) || py>=bottom || py>=720) continue;
            const auto alpha=g.bitmap.buffer[row*g.bitmap.pitch+col];
            auto* p=pixels.data()+(py*1280+px)*4;
            for(int channel=0;channel<3;++channel) p[channel]=std::byte((std::to_integer<unsigned>(p[channel])*(255-alpha)+255*alpha)/255);
        }
        pen+=advance;
    }
}
std::span<const std::byte> MessageDialogUI::Draw() {
    const auto previousGeneration=view.generation;
    active=MsgDialogView_nid_no_patch(&view);
    if(view.generation!=previousGeneration) { horizontal=0; vertical=0; }
    if(!active || !font->face) return {};
    rect(0,0,1280,720,0x071428);
    rect(96,132,1088,2,0x586981);
    rect(96,560,1088,2,0x586981);
    const int maxScroll=std::max(0,Lines(font->face,view.message,1024)-9);
    MsgDialogLayout_nid_no_patch(view.generation,maxScroll);
    view.scroll=std::min(view.scroll,maxScroll);
    text(view.message,128,180-view.scroll*36,1024,510,180);
    if(view.mode==2) {
        rect(128,520,928,12,0x33445c);
        rect(128,520,928*view.progress/100,12,0xddeaff);
        text(std::to_string(view.progress)+"%",1080,508,100,548);
    }
    for(int i=0;i<view.count;++i) {
        const int x=view.count==1?480:(i==0?656:304);
        if(i==view.selected) { rect(x-3,594,326,58,0xf1f5ff); rect(x,597,320,52,0x294979); }
        else rect(x,597,320,52,0x162a47);
        text(view.buttons[i],x+22,606,276,645);
    }
    text("Enter / Cross: select    Esc / Circle: cancel    Arrows: navigate / scroll",128,668,1024,712);
    return pixels;
}
void MessageDialogUI::Presented() { if(active) MsgDialogPresented_nid_no_patch(view.generation); }
bool MessageDialogUI::HandleEvent(const SDL_Event& e,std::uint32_t windowId) {
    MsgDialog::View current;
    if(!MsgDialogView_nid_no_patch(&current)) return false;
    const auto send=[&](MsgDialog::Action a) { MsgDialogInput_nid_no_patch(current.generation,a); };
    using MsgDialog::Action;
    if(e.type==SDL_KEYDOWN && e.key.windowID==windowId) {
        if(e.key.repeat && (e.key.keysym.sym==SDLK_RETURN || e.key.keysym.sym==SDLK_KP_ENTER || e.key.keysym.sym==SDLK_ESCAPE || e.key.keysym.sym==SDLK_SPACE)) return true;
        switch(e.key.keysym.sym) {
        case SDLK_LEFT: send(Action::Previous); break;
        case SDLK_RIGHT: case SDLK_TAB: send(Action::Next); break;
        case SDLK_UP: send(Action::ScrollUp); break;
        case SDLK_DOWN: send(Action::ScrollDown); break;
        case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_SPACE: send(Action::Confirm); break;
        case SDLK_ESCAPE: send(Action::Cancel); break;
        }
        return true;
    }
    if(e.type==SDL_CONTROLLERBUTTONDOWN) {
        switch(e.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: send(Action::Previous); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: send(Action::Next); break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: send(Action::ScrollUp); break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: send(Action::ScrollDown); break;
        case SDL_CONTROLLER_BUTTON_A: send(Action::Confirm); break;
        case SDL_CONTROLLER_BUTTON_B: send(Action::Cancel); break;
        }
        return true;
    }
    if(e.type==SDL_CONTROLLERAXISMOTION && (e.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX || e.caxis.axis==SDL_CONTROLLER_AXIS_LEFTY)) {
        auto& direction=e.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX?horizontal:vertical;
        const int next=e.caxis.value>18000?1:e.caxis.value< -18000?-1:0;
        if(next && next!=direction) {
            send(e.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX?(next>0?Action::Next:Action::Previous):(next>0?Action::ScrollDown:Action::ScrollUp));
        }
        direction=next; return true;
    }
    return false;
}
