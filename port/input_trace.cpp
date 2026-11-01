// SPDX-License-Identifier: GPL-3.0-only
#include "input_trace.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace port {
namespace {
// All calls are on the SDL/main thread, including the native input sink.
struct Trace {
    FILE *file = nullptr;
    uint64_t sequence = 0, frame = 0, bytes = 0, lastFlush = 0;
    KeyboardConfig config;
    bool configured = false;
    std::map<std::string,int> devices;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    uint64_t Now() const { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count(); }
    Trace() {
        const char *directory = std::getenv("CPS2_KEYBOARD_TRACE_DIR");
        if (!directory || !*directory) return;
        std::error_code ec;
        const auto folder = std::filesystem::u8path(directory);
        std::filesystem::create_directories(folder,ec);
        if(ec) return;
#ifdef _WIN32
        wchar_t module[32768]{}; GetModuleFileNameW(nullptr,module,32768);
        const auto path=folder/(std::filesystem::path(module).stem().wstring()+L"_"+std::to_wstring(GetCurrentProcessId())+L".csv");
        file=_wfopen(path.c_str(),L"wb");
#else
        file=std::fopen((folder/"input_trace.csv").c_str(),"wb");
#endif
        if(file) {
            std::setvbuf(file,nullptr,_IOFBF,65536);
            std::fputs("seq,host_ns,frame,kind,device,player,scan,make,flags,vk,down,repeat,focused,keys,physical,blocked,mods,keyboard_mask,gamepad_mask,suppressed_mask,output_mask,menu,paused,detail\n",file);
            const auto epoch=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            Write("SESSION","",0,SDL_SCANCODE_UNKNOWN,0,0,0,false,false,-1,"","","",0,0,0,0,0,false,false,
                  "unix_ns="+std::to_string(epoch)+";sdl_version="+std::to_string(SDL_GetVersion()));
        }
    }
    ~Trace() { if(file) std::fclose(file); }
    std::string Device(const std::string &id) {
        if(id.empty())return "";
        auto found=devices.find(id);
        if(found==devices.end()) found=devices.emplace(id,static_cast<int>(devices.size())+1).first;
        return "K"+std::to_string(found->second);
    }
    int Player(const std::string &id) const {
        if(!id.empty()) for(int p=0;p<2;++p) if(config.device[p]==id)return p+1;
        return 0;
    }
    static std::string Quote(const std::string &value) {
        std::string result="\"";
        for(char c:value) {if(c=='\"')result+='\"'; if(c!='\r'&&c!='\n')result+=c;}
        return result+'\"';
    }
    void Write(const char *kind,const std::string &id,int player,SDL_Scancode scan,unsigned make,unsigned flags,unsigned vk,
               bool down,bool repeat,int focused,const std::string &keys,const std::string &physical,const std::string &blocked,
               unsigned mods,uint32_t keyboard,uint32_t gamepad,uint32_t suppressed,uint32_t output,bool menu,bool paused,const std::string &detail) {
        if(!file)return;
        const auto now=Now();
        if(bytes>64u*1024*1024 || now>1200000000000ull) {
            std::fputs("# TRACE_LIMIT: twenty minutes or 64 MiB reached; start another capture.\n",file);
            std::fclose(file);file=nullptr;return;
        }
        const int count=std::fprintf(file,"%llu,%llu,%llu,%s,%s,%d,%d,%u,%u,%u,%d,%d,%d,%s,%s,%s,%u,%x,%x,%x,%x,%d,%d,%s\n",
            static_cast<unsigned long long>(++sequence),static_cast<unsigned long long>(now),static_cast<unsigned long long>(frame),kind,
            Device(id).c_str(),player,static_cast<int>(scan),make,flags,vk,down,repeat,focused,Quote(keys).c_str(),Quote(physical).c_str(),
            Quote(blocked).c_str(),mods,keyboard,gamepad,suppressed,output,menu,paused,Quote(detail).c_str());
        if(count>0)bytes+=count;
        if(now-lastFlush>1000000000ull) {std::fflush(file);lastFlush=now;}
    }
};
Trace &Log() {static Trace trace;return trace;}
std::string Keys(const KeyboardState::Keys &keys) {
    std::string result;
    for(int k=1;k<SDL_SCANCODE_COUNT;++k)if(keys[k]) {
        if(!result.empty())result+='+';
        const char *name=SDL_GetScancodeName(static_cast<SDL_Scancode>(k));
        result+=name&&*name?name:std::to_string(k);
    }
    return result;
}
std::string Hex(uint32_t value) {char text[16];std::snprintf(text,sizeof(text),"%x",value);return text;}
}
bool InputTraceEnabled(){return Log().file!=nullptr;}
void InputTraceFrame(uint64_t frame){if(InputTraceEnabled())Log().frame=frame;}
void InputTraceNote(const char *kind,const std::string &detail) {
    if(InputTraceEnabled())Log().Write(kind,"",0,SDL_SCANCODE_UNKNOWN,0,0,0,false,false,-1,"","","",0,0,0,0,0,false,false,detail);
}
void InputTraceConfigure(const KeyboardConfig &config) {
    if(!InputTraceEnabled())return;
    auto &t=Log();
    if(t.configured && t.config.enabled==config.enabled && t.config.mirrorP1==config.mirrorP1 && t.config.device==config.device)return;
    t.config=config;t.configured=true;
    InputTraceNote("CONFIG","separate="+std::to_string(config.enabled)+";mirror="+std::to_string(config.mirrorP1)+
        ";p1="+t.Device(config.device[0])+";p2="+t.Device(config.device[1]));
}
void InputTraceFocus(bool focused) {
    if(InputTraceEnabled())Log().Write("FOCUS","",0,SDL_SCANCODE_UNKNOWN,0,0,0,false,false,focused,"","","",0,0,0,0,0,false,false,"");
}
void InputTraceDevice(const KeyboardState &state,const std::string &id) {
    if(!InputTraceEnabled())return;
    auto &t=Log();const auto it=state.Devices().find(id);
    t.Write("DEVICE",id,state.PlayerFor(t.config,id),SDL_SCANCODE_UNKNOWN,0,0,0,false,false,-1,"","","",0,0,0,0,0,false,false,
        "physical="+t.Device(state.PhysicalID(id))+";name="+(it==state.Devices().end()?"":it->second.name));
}
void InputTraceRaw(const KeyboardState &state,const std::string &id,SDL_Scancode key,unsigned make,unsigned flags,unsigned vk,
                   bool down,bool repeat,uint32_t messageMs) {
    if(!InputTraceEnabled())return;
    auto &t=Log();const auto found=state.Devices().find(id);
    const bool visible=state.Focused();
    const auto *d=found==state.Devices().end()?nullptr:&found->second;
    // Background raw input is counted without logging the user's desktop keys.
    t.Write(visible?"RAW":"RAW_BACKGROUND",id,state.PlayerFor(t.config,id),visible?key:SDL_SCANCODE_UNKNOWN,visible?make:0,visible?flags:0,visible?vk:0,
        visible&&down,visible&&repeat,visible,visible&&d?Keys(d->keys):"",visible&&d?Keys(d->physical):"",visible&&d?Keys(d->blocked):"",
        visible&&d?state.Modifiers(d->keys):0,0,0,0,0,false,false,"msg_ms="+std::to_string(messageMs));
}
void InputTraceSDL(SDL_Scancode key,bool down,bool repeat,SDL_Keymod mods,uint32_t keyboard) {
    if(InputTraceEnabled())Log().Write("SDL","",0,key,0,0,0,down,repeat,-1,"","","",mods,0,0,0,0,false,false,
                                      "sdl_keyboard="+std::to_string(keyboard));
}
void InputTracePlayer(const KeyboardState &state,const KeyboardConfig &c,int player,uint32_t keyboard,uint32_t gamepad,
                      uint32_t suppressed,uint16_t output,bool menu,bool paused) {
    if(!InputTraceEnabled())return;
    auto &t=Log();const auto snapshot=state.Snapshot(c,player);
    const auto *d=&snapshot;
    const bool visible=state.Focused();
    t.Write("SAMPLE",c.device[player],player+1,SDL_SCANCODE_UNKNOWN,0,0,0,false,false,visible,
        visible&&d?Keys(d->keys):"",visible&&d?Keys(d->physical):"",visible&&d?Keys(d->blocked):"",
        visible&&d?state.Modifiers(d->keys):0,keyboard,gamepad,suppressed,output,menu,paused,
        "connected="+std::to_string(d&&d->connected)+";interfaces="+std::to_string(state.Interfaces(c.device[player])));
}
void InputTraceProfile(const InputProfile &profile,const Bindings &bindings) {
    if(!InputTraceEnabled())return;
    static std::array<std::string,2> previous;
    for(int p=0;p<2;++p) {
        std::string detail="player="+std::to_string(p+1)+";neutral="+std::to_string(profile.players[p].neutralize)+
            ";delay="+std::to_string(profile.players[p].autofireDelay)+";autofire=";
        unsigned mask=0;for(int a=0;a<kInputAttackActions;++a)if(profile.players[p].autofire[a])mask|=1u<<a;
        detail+=Hex(mask);
        for(int a=0;a<kInputCustomCount;++a)detail+=";custom"+std::to_string(a+1)+"="+Hex(profile.players[p].custom[a]);
        for(int a=0;a<kBoundActionCount;++a)detail+=";"+std::string(kBoundActions[a].name)+"="+recomp::KeyText(bindings.key[p][a]);
        if(previous[p]!=detail) {InputTraceNote("PROFILE",detail);previous[p]=detail;}
    }
}
void InputTraceGame(const char *stage,uint64_t frame,uint16_t one,uint16_t two,uint16_t io0,uint16_t io1,uint16_t io2) {
    if(!InputTraceEnabled())return;
    Log().frame=frame;
    Log().Write(stage,"",0,SDL_SCANCODE_UNKNOWN,0,0,0,false,false,-1,"","","",0,one,two,0,0,false,false,
        "p1="+Hex(one)+";p2="+Hex(two)+";io0="+Hex(io0)+";io1="+Hex(io1)+";io2="+Hex(io2));
}
} // namespace port
