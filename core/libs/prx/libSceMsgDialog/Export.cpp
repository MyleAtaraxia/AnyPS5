#include "prx/libSceMsgDialog/MsgDialog.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
using MsgDialog::Operation;
extern "C" {
int APS5_VABI sceMsgDialogInitialize() { return MsgDialogCall_nid_no_patch(Operation::Initialize,nullptr,0,0); }
int APS5_VABI sceMsgDialogTerminate() { return MsgDialogCall_nid_no_patch(Operation::Terminate,nullptr,0,0); }
int APS5_VABI sceMsgDialogOpen(const void* p) { return MsgDialogCall_nid_no_patch(Operation::Open,p,0,0); }
int APS5_VABI sceMsgDialogClose() { return MsgDialogCall_nid_no_patch(Operation::Close,nullptr,0,0); }
int APS5_VABI sceMsgDialogGetStatus() { return MsgDialogCall_nid_no_patch(Operation::GetStatus,nullptr,0,0); }
int APS5_VABI sceMsgDialogUpdateStatus() { return sceMsgDialogGetStatus(); }
int APS5_VABI sceMsgDialogGetResult(MsgDialogResult* p) { return MsgDialogCall_nid_no_patch(Operation::GetResult,p,0,0); }
int APS5_VABI sceMsgDialogProgressBarInc(int t, std::uint32_t d) { return MsgDialogCall_nid_no_patch(Operation::Inc,nullptr,t,d); }
int APS5_VABI sceMsgDialogProgressBarSetValue(int t, std::uint32_t v) { return MsgDialogCall_nid_no_patch(Operation::SetValue,nullptr,t,v); }
int APS5_VABI sceMsgDialogProgressBarSetMsg(int t, const char* p) { return MsgDialogCall_nid_no_patch(Operation::SetMessage,p,t,0); }
}
