#pragma once
#include "prx/libSceMsgDialog/MsgDialog.hpp"
#include "SDL_events.h"
#include <span>
#include <vector>
#include <cstddef>
#include <memory>
class MessageDialogUI {
public:
    MessageDialogUI();
    ~MessageDialogUI();
    bool HandleEvent(const SDL_Event&, std::uint32_t windowId);
    std::span<const std::byte> Draw();
    void Presented();
    bool Active() const { return active; }
private:
    struct Font;
    std::unique_ptr<Font> font;
    MsgDialog::View view;
    std::vector<std::byte> pixels;
    bool active=false;
    int horizontal=0, vertical=0;
    void rect(int,int,int,int,unsigned);
    void text(const std::string&,int,int,int,int,int clipTop=0);
};
