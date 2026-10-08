#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include "SceTypes.hpp"

namespace MsgDialog {
struct BaseParam { std::size_t size; std::array<char,36> reserved; std::uint32_t magic; };
struct ButtonsParam { const char* msg1; const char* msg2; std::array<char,32> reserved; };
struct UserParam { std::int32_t buttonType; std::int32_t padding; const char* msg; const ButtonsParam* buttons; std::array<char,24> reserved; };
struct ProgressParam { std::int32_t barType; std::int32_t padding; const char* msg; std::array<char,64> reserved; };
struct SystemParam { std::int32_t type; std::array<char,32> reserved; };
struct Param {
    BaseParam base; std::size_t size; std::int32_t mode; std::int32_t padding;
    const UserParam* user; const ProgressParam* progress; const SystemParam* system;
    std::int32_t userId; std::array<char,40> reserved; std::int32_t padding2;
};
static_assert(sizeof(BaseParam)==48 && sizeof(Param)==136);
static_assert(offsetof(Param,mode)==0x38 && offsetof(Param,user)==0x40);
static_assert(sizeof(UserParam)==48 && sizeof(ProgressParam)==80 && sizeof(MsgDialogResult)==44);
enum class Operation { Initialize, Terminate, Open, Close, GetStatus, GetResult, Inc, SetValue, SetMessage };
enum class Action { Previous, Next, Confirm, Cancel, ScrollUp, ScrollDown };
enum class Phase { Closed, Open, Running, Finished };
struct View {
    std::uint64_t generation{}; Phase phase{Phase::Closed}; int mode{}, type{}, selected{}, count{}, scroll{}, maxScroll{};
    std::uint32_t progress{}; bool cancellable{};
    std::string message; std::array<std::string,2> buttons;
};
}
extern "C" int MsgDialogCall_nid_no_patch(MsgDialog::Operation, const void*, int, std::uint32_t);
extern "C" bool MsgDialogView_nid_no_patch(MsgDialog::View*);
extern "C" void MsgDialogPresented_nid_no_patch(std::uint64_t);
extern "C" void MsgDialogLayout_nid_no_patch(std::uint64_t, int);
extern "C" void MsgDialogInput_nid_no_patch(std::uint64_t, MsgDialog::Action);
extern "C" void MsgDialogRenderer_nid_no_patch(bool);
