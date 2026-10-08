#include "prx/libSceMsgDialog/MsgDialog.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <limits>
extern "C" {
int APS5_VABI sceCommonDialogInitialize();
bool APS5_VABI sceCommonDialogIsUsed();
int APS5_VABI sceMsgDialogInitialize();
int APS5_VABI sceMsgDialogOpen(const void*);
int APS5_VABI sceMsgDialogGetStatus();
int APS5_VABI sceMsgDialogUpdateStatus();
int APS5_VABI sceMsgDialogGetResult(MsgDialogResult*);
int APS5_VABI sceMsgDialogClose();
int APS5_VABI sceMsgDialogTerminate();
int APS5_VABI sceMsgDialogProgressBarInc(int,std::uint32_t);
int APS5_VABI sceMsgDialogProgressBarSetValue(int,std::uint32_t);
int APS5_VABI sceMsgDialogProgressBarSetMsg(int,const char*);
}
#define CHECK(expr) do { if(!(expr)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#expr); std::abort(); } } while(0)
constexpr int E(unsigned n) { return static_cast<int>(0x80B80000u+n); }
using namespace MsgDialog;
int main() {
    MsgDialog::Param p{}; p.size=sizeof(p); p.base.size=sizeof(p.base); p.mode=1;
    UserParam u{}; u.msg="Message"; p.user=&u;
    CHECK(sceMsgDialogGetStatus()==0);
    CHECK(sceMsgDialogInitialize()==E(1));
    CHECK(sceMsgDialogOpen(&p)==E(6));
    CHECK(sceMsgDialogClose()==E(11));
    CHECK(sceMsgDialogTerminate()==E(3));
    CHECK(sceMsgDialogGetResult(nullptr)==E(5));
    CHECK(sceCommonDialogInitialize()==0);
    CHECK(sceCommonDialogInitialize()==E(2));
    CHECK(sceMsgDialogInitialize()==0);
    CHECK(sceCommonDialogIsUsed());
    CHECK(sceMsgDialogInitialize()==E(4));
    CHECK(sceMsgDialogOpen(nullptr)==E(13));
    CHECK(sceMsgDialogOpen(reinterpret_cast<const void*>(1))==E(10));
    CHECK(sceMsgDialogOpen(&p)==E(15));
    MsgDialogRenderer_nid_no_patch(true);
    p.size=0; CHECK(sceMsgDialogOpen(&p)==E(10)); p.size=sizeof(p);
    p.base.size=0; CHECK(sceMsgDialogOpen(&p)==E(10)); p.base.size=sizeof(p.base);
    p.mode=0; CHECK(sceMsgDialogOpen(&p)==E(10)); p.mode=1;
    p.user=nullptr; CHECK(sceMsgDialogOpen(&p)==E(13)); p.user=&u;
    u.msg=reinterpret_cast<const char*>(1); CHECK(sceMsgDialogOpen(&p)==E(10));
    u.msg=nullptr; CHECK(sceMsgDialogOpen(&p)==E(13)); u.msg="Message";
    u.buttonType=4; CHECK(sceMsgDialogOpen(&p)==E(10));
    u.buttonType=10; CHECK(sceMsgDialogOpen(&p)==E(10));
    ButtonsParam b{}; b.msg1="Continue"; b.msg2="Back"; u.buttons=&b;
    MsgDialogResult r{};
    std::uint64_t stale=0;
    for(int type: {0,1,2,3,5,6,7,8,9}) {
        u.buttonType=type;
        CHECK(sceMsgDialogOpen(&p)==0);
        CHECK(sceMsgDialogOpen(&p)==E(6));
        CHECK(sceMsgDialogGetResult(&r)==E(5));
        CHECK(sceMsgDialogProgressBarSetValue(0,20)==E(15));
        View v; CHECK(MsgDialogView_nid_no_patch(&v));
        CHECK(v.phase==Phase::Open);
        CHECK(v.selected==((type==7 || type==8)?1:0));
        MsgDialogInput_nid_no_patch(v.generation,Action::Confirm);
        CHECK(sceMsgDialogGetStatus()==2);
        MsgDialogPresented_nid_no_patch(v.generation);
        MsgDialogInput_nid_no_patch(stale,Action::Confirm);
        CHECK(sceMsgDialogGetStatus()==2);
        for(int i=0;i<200;++i) CHECK(sceMsgDialogUpdateStatus()==2);
        MsgDialogInput_nid_no_patch(v.generation,Action::Cancel);
        if(v.cancellable) {
            CHECK(sceMsgDialogGetStatus()==3);
            CHECK(sceMsgDialogGetResult(&r)==0 && r.result==1 && r.button_id==0);
        } else {
            CHECK(sceMsgDialogGetStatus()==2);
            MsgDialogInput_nid_no_patch(v.generation,Action::Confirm);
            if(type==2) { CHECK(sceMsgDialogGetStatus()==2); CHECK(sceMsgDialogClose()==0); }
            CHECK(sceMsgDialogGetResult(&r)==0);
            CHECK(r.mode==1 && r.result==0);
            CHECK(r.button_id==(type==2?0:type==7?2:1));
        }
        CHECK(sceMsgDialogGetResult(nullptr)==E(13));
        CHECK(sceMsgDialogGetResult(reinterpret_cast<MsgDialogResult*>(1))==E(10));
        CHECK(sceMsgDialogClose()==E(11));
        for(char c:r.reserved) CHECK(c==0);
        CHECK(!MsgDialogView_nid_no_patch(&v));
        stale=v.generation;
    }
    for(int type: {1,3,6,7,8,9}) {
        u.buttonType=type; CHECK(sceMsgDialogOpen(&p)==0);
        View v; CHECK(MsgDialogView_nid_no_patch(&v)); MsgDialogPresented_nid_no_patch(v.generation);
        if(v.selected==0) MsgDialogInput_nid_no_patch(v.generation,Action::Next);
        MsgDialogInput_nid_no_patch(v.generation,Action::Confirm);
        CHECK(sceMsgDialogGetResult(&r)==0);
        const bool cancel=type==3 || type==6 || type==8;
        CHECK(r.result==(cancel?1:0) && r.button_id==(cancel?0:2));
    }
    u.buttonType=9; u.buttons=nullptr; CHECK(sceMsgDialogOpen(&p)==E(13)); u.buttons=&b;
    b.msg2=nullptr; CHECK(sceMsgDialogOpen(&p)==E(13)); b.msg2="Back";
    std::string oversized(65536,'a'); u.msg=oversized.c_str(); CHECK(sceMsgDialogOpen(&p)==E(10));
    std::string owned="Owned message"; u.msg=owned.c_str(); u.buttonType=0;
    CHECK(sceMsgDialogOpen(&p)==0); owned.assign("Changed");
    View v; CHECK(MsgDialogView_nid_no_patch(&v)); CHECK(v.message=="Owned message");
    CHECK(sceMsgDialogClose()==0); u.msg="Message";
    ProgressParam progress{}; progress.msg="Loading"; p.mode=2; p.progress=&progress;
    for(int type:{0,1}) {
        progress.barType=type; CHECK(sceMsgDialogOpen(&p)==0);
        CHECK(sceMsgDialogProgressBarSetValue(1,50)==E(10));
        CHECK(sceMsgDialogProgressBarSetValue(0,101)==E(10));
        CHECK(sceMsgDialogProgressBarSetMsg(0,nullptr)==E(13));
        CHECK(sceMsgDialogProgressBarSetValue(0,40)==0);
        CHECK(sceMsgDialogProgressBarInc(0,std::numeric_limits<std::uint32_t>::max())==0);
        CHECK(sceMsgDialogProgressBarSetMsg(0,"Complete")==0);
        CHECK(MsgDialogView_nid_no_patch(&v)); CHECK(v.progress==100 && v.message=="Complete");
        MsgDialogPresented_nid_no_patch(v.generation);
        CHECK(sceMsgDialogUpdateStatus()==2);
        MsgDialogInput_nid_no_patch(v.generation,Action::Cancel);
        if(type==0) { CHECK(sceMsgDialogGetStatus()==2); CHECK(sceMsgDialogClose()==0); }
        CHECK(sceMsgDialogGetResult(&r)==0 && r.result==type && r.button_id==0);
    }
    progress.barType=2; CHECK(sceMsgDialogOpen(&p)==E(10));
    SystemParam system{}; p.mode=3; p.system=&system;
    for(int type:{0,1,2,4}) {
        system.type=type; CHECK(sceMsgDialogOpen(&p)==0);
        CHECK(MsgDialogView_nid_no_patch(&v)); CHECK(!v.message.empty());
        MsgDialogPresented_nid_no_patch(v.generation); MsgDialogInput_nid_no_patch(v.generation,Action::Confirm);
        CHECK(sceMsgDialogGetResult(&r)==0 && r.button_id==1);
    }
    system.type=3; CHECK(sceMsgDialogOpen(&p)==E(10));
    system.type=5; CHECK(sceMsgDialogOpen(&p)==E(15));
    p.mode=1; u.buttonType=0; CHECK(sceMsgDialogOpen(&p)==0);
    CHECK(MsgDialogView_nid_no_patch(&v));
    std::thread poller([]{ for(int i=0;i<10000;++i) { int s=sceMsgDialogGetStatus(); CHECK(s==0 || s==2 || s==3); } });
    std::thread input([&]{ for(int i=0;i<1000;++i) { MsgDialogPresented_nid_no_patch(v.generation); MsgDialogInput_nid_no_patch(v.generation,Action::Confirm); } });
    CHECK(sceMsgDialogTerminate()==0); poller.join(); input.join();
    CHECK(!sceCommonDialogIsUsed()); CHECK(sceMsgDialogGetStatus()==0);
    CHECK(sceMsgDialogProgressBarInc(0,1)==E(11));
    CHECK(sceMsgDialogInitialize()==0); CHECK(sceMsgDialogOpen(&p)==0);
    MsgDialogRenderer_nid_no_patch(false);
    CHECK(sceMsgDialogGetResult(&r)==0 && r.result==1);
    CHECK(sceMsgDialogTerminate()==0);
}
