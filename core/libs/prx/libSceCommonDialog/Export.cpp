#include "prx/libSceCommonDialog/CommonDialog.hpp"
#include "prx/libSceMsgDialog/MsgDialog.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <new>
#include <limits>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_vm.h>
#endif

namespace {
constexpr int Error(unsigned n) { return static_cast<int>(0x80B80000u+n); }
std::mutex mutex;
bool initialized=false, renderer=false;
int status=0;
MsgDialog::View view;
MsgDialogResult result{};
std::uint64_t generation=0;
std::size_t Accessible(const void* p, bool writable=false) {
    if (!p) return 0;
    const auto address=reinterpret_cast<std::uintptr_t>(p);
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(p,&info,sizeof(info)) || info.State!=MEM_COMMIT || (info.Protect&(PAGE_NOACCESS|PAGE_GUARD))) return 0;
    if (writable && !(info.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))) return 0;
    return reinterpret_cast<std::uintptr_t>(info.BaseAddress)+info.RegionSize-address;
#elif defined(__APPLE__)
    mach_vm_address_t start=address; mach_vm_size_t size=0;
    vm_region_basic_info_data_64_t info{}; mach_msg_type_number_t count=VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object{};
    const auto code=mach_vm_region(mach_task_self(),&start,&size,VM_REGION_BASIC_INFO_64,reinterpret_cast<vm_region_info_t>(&info),&count,&object);
    if(object) mach_port_deallocate(mach_task_self(),object);
    if(code!=KERN_SUCCESS || start>address || !(info.protection&VM_PROT_READ) || (writable && !(info.protection&VM_PROT_WRITE))) return 0;
    return start+size-address;
#else
    std::FILE* maps=std::fopen("/proc/self/maps","r");
    if (!maps) return 0;
    char line[1024]; std::size_t available=0;
    while(std::fgets(line,sizeof(line),maps)) {
        unsigned long long begin=0,end=0; char flags[5]{};
        if(std::sscanf(line,"%llx-%llx %4s",&begin,&end,flags)==3 && address>=begin && address<end) {
            if(flags[0]=='r' && (!writable || flags[1]=='w')) available=end-address;
            break;
        }
    }
    std::fclose(maps); return available;
#endif
}
bool Range(const void* p,std::size_t bytes,bool writable=false) {
    auto address=reinterpret_cast<std::uintptr_t>(p);
    if(bytes>std::numeric_limits<std::uintptr_t>::max()-address) return false;
    while(bytes) {
        const auto n=std::min(bytes,Accessible(reinterpret_cast<const void*>(address),writable));
        if(!n) return false;
        address+=n; bytes-=n;
    }
    return true;
}
template<class T> int Read(const T* p,T& out) {
    if(!p) return Error(13);
    if(!Range(p,sizeof(T))) return Error(10);
    std::memcpy(&out,p,sizeof(T)); return 0;
}
int Copy(const char* p,std::string& out) {
    if (!p) return Error(13);
    constexpr std::size_t limit=65536;
    std::string next;
    for(std::size_t offset=0;offset<limit;) {
        const auto n=std::min(limit-offset,Accessible(p+offset));
        if(!n) return Error(10);
        const auto* end=static_cast<const char*>(std::memchr(p+offset,0,n));
        next.append(p+offset,end?static_cast<std::size_t>(end-p-offset):n);
        if(end) { out=std::move(next); return 0; }
        offset+=n;
    }
    return Error(10);
}
void Finish(int button=0, int code=0) {
    result={}; result.mode=view.mode; result.result=code; result.button_id=button;
    status=3; view.phase=MsgDialog::Phase::Finished;
}
int Prepare(const MsgDialog::Param& p, MsgDialog::View& next) {
    if (p.size!=sizeof(p) || p.base.size!=sizeof(p.base)) return Error(10);
    next.mode=p.mode;
    if (p.mode==1) {
        MsgDialog::UserParam u{};
        if(const int err=Read(p.user,u)) return err; next.type=u.buttonType;
        if (u.buttonType<0 || u.buttonType>9 || u.buttonType==4) return Error(10);
        if (const int err=Copy(u.msg,next.message)) return err;
        switch (u.buttonType) {
        case 0: next.buttons={"OK",""}; next.count=1; break;
        case 1: case 7: next.buttons={"Yes","No"}; next.count=2; break;
        case 2: break;
        case 3: case 8: next.buttons={"OK","Cancel"}; next.count=2; next.cancellable=true; break;
        case 5: next.buttons={"Wait",""}; next.count=1; break;
        case 6: next.buttons={"Wait","Cancel"}; next.count=2; next.cancellable=true; break;
        case 9: {
            MsgDialog::ButtonsParam buttons;
            if(const int err=Read(u.buttons,buttons)) return err;
            if (const int err=Copy(buttons.msg1,next.buttons[0])) return err;
            if (const int err=Copy(buttons.msg2,next.buttons[1])) return err;
            next.count=2; break;
        }
        }
        next.selected=(u.buttonType==7 || u.buttonType==8)?1:0;
    } else if (p.mode==2) {
        MsgDialog::ProgressParam progress;
        if(const int err=Read(p.progress,progress)) return err;
        if (progress.barType<0 || progress.barType>1) return Error(10);
        next.type=progress.barType;
        if (const int err=Copy(progress.msg,next.message)) return err;
        next.cancellable=next.type==1;
        if (next.cancellable) { next.count=1; next.buttons[0]="Cancel"; }
    } else if (p.mode==3) {
        MsgDialog::SystemParam system;
        if(const int err=Read(p.system,system)) return err;
        next.type=system.type;
        switch(next.type) {
        case 0: next.message="There are no products available in the store."; break;
        case 1: next.message="Communication features are restricted for this account."; break;
        case 2: next.message="User-generated content is restricted for this account."; break;
        case 4: next.message="The camera is not connected."; break;
        case 5: return Error(15);
        default: return Error(10);
        }
        next.count=1; next.buttons[0]="OK";
    } else return Error(10);
    return 0;
}
}
extern "C" int APS5_VABI sceCommonDialogInitialize() {
    std::lock_guard lock(mutex);
    if (initialized) return Error(2);
    initialized=true; return 0;
}
extern "C" bool APS5_VABI sceCommonDialogIsUsed() {
    std::lock_guard lock(mutex); return status!=0;
}
extern "C" int MsgDialogCall_nid_no_patch(MsgDialog::Operation op,const void* p,int target,std::uint32_t value) {
    using MsgDialog::Operation;
    std::lock_guard lock(mutex);
    try {
        switch(op) {
        case Operation::Initialize:
            if (!initialized) return Error(1);
            if (status!=0) return Error(4);
            status=1; return 0;
        case Operation::Terminate:
            if (status==0) return Error(3);
            status=0; view={}; ++generation; result={}; return 0;
        case Operation::GetStatus: return status;
        case Operation::GetResult:
            if (status!=3) return Error(5);
            if (!p) return Error(13);
            if(!Range(p,sizeof(result),true)) return Error(10);
            *static_cast<MsgDialogResult*>(const_cast<void*>(p))=result; return 0;
        case Operation::Open: {
            if (status!=1 && status!=3) return Error(6);
            MsgDialog::Param param;
            if(const int err=Read(static_cast<const MsgDialog::Param*>(p),param)) return err;
            MsgDialog::View next;
            if (const int err=Prepare(param,next)) return err;
            if (!renderer) return Error(15);
            next.generation=++generation; next.phase=MsgDialog::Phase::Open;
            view=std::move(next); result={}; status=2; return 0;
        }
        case Operation::Close:
            if (status!=2) return Error(11);
            Finish(); return 0;
        default:
            if (status!=2) return Error(11);
            if (view.mode!=2) return Error(15);
            if (target!=0) return Error(10);
            if (op==Operation::SetMessage) return Copy(static_cast<const char*>(p),view.message);
            if (op==Operation::SetValue) {
                if (value>100) return Error(10);
                view.progress=value;
            } else if (op==Operation::Inc) {
                view.progress+=std::min(value,100-view.progress);
            } else return Error(15);
            return 0;
        }
    } catch(const std::bad_alloc&) { return Error(9); }
}
extern "C" bool MsgDialogView_nid_no_patch(MsgDialog::View* out) {
    std::lock_guard lock(mutex);
    if (status!=2) { if (view.phase==MsgDialog::Phase::Finished) view.phase=MsgDialog::Phase::Closed; return false; }
    *out=view; return true;
}
extern "C" void MsgDialogPresented_nid_no_patch(std::uint64_t token) {
    std::lock_guard lock(mutex);
    if (status==2 && token==view.generation) view.phase=MsgDialog::Phase::Running;
}
extern "C" void MsgDialogLayout_nid_no_patch(std::uint64_t token, int maxScroll) {
    std::lock_guard lock(mutex);
    if(status==2 && token==view.generation) {
        view.maxScroll=std::max(0,maxScroll);
        view.scroll=std::min(view.scroll,view.maxScroll);
    }
}
extern "C" void MsgDialogInput_nid_no_patch(std::uint64_t token,MsgDialog::Action action) {
    using MsgDialog::Action;
    std::lock_guard lock(mutex);
    if (status!=2 || token!=view.generation || view.phase!=MsgDialog::Phase::Running) return;
    switch(action) {
    case Action::Previous: case Action::Next:
        if (view.count==2) view.selected=1-view.selected; break;
    case Action::Cancel:
        if (view.cancellable) Finish(0,1); break;
    case Action::Confirm:
        if (view.count==0) break;
        if (view.mode==2 || (view.cancellable && view.selected==1)) Finish(0,1);
        else Finish(view.selected+1);
        break;
    case Action::ScrollUp: view.scroll=std::max(0,view.scroll-1); break;
    case Action::ScrollDown: view.scroll=std::min(view.maxScroll,view.scroll+1); break;
    }
}
extern "C" void MsgDialogRenderer_nid_no_patch(bool available) {
    std::lock_guard lock(mutex); renderer=available;
    if (!available && status==2) Finish(0,1);
}
