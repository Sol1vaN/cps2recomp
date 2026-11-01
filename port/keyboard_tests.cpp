// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026. Local Input Lab extension to cps2recomp.
#include "raw_keyboards.h"
#include "input_trace.h"
#include "../recomp-common/settings.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace port;
namespace {
int checks = 0;
void Check(bool ok, const char *what) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAILED: %s\n", what); std::exit(1); }
}
std::string Contents(const std::filesystem::path &p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
KeyboardConfig Config() { return {true, true, {"physical-A", "physical-B"}}; }
Bindings SameKeys() {
    auto b = DefaultBindings();
    constexpr SDL_Scancode keys[] = {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D,
        SDL_SCANCODE_T, SDL_SCANCODE_Y, SDL_SCANCODE_U, SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_J,
        SDL_SCANCODE_RETURN, SDL_SCANCODE_1, SDL_SCANCODE_I, SDL_SCANCODE_K, SDL_SCANCODE_R,
        SDL_SCANCODE_F, SDL_SCANCODE_V, SDL_SCANCODE_B};
    for (int p = 0; p < 2; ++p) for (int a = 0; a < kBoundActionCount; ++a) b.key[p][a] = keys[a];
    return b;
}
void Routing() {
    const auto c = Config(); const auto b = SameKeys(); const Hotkeys none{};
    KeyboardState s; s.Connect(c.device[0], "Identical keyboard"); s.Connect(c.device[1], "Identical keyboard"); s.Focus(true);
    const auto buttons = [&](int p) { const auto &k = s.ForPlayer(c, p);
        return ReadKeyboardButtons(b, k.data(), 0, none, s.Modifiers(k), p == 0); };
    // Every pair of game actions, both release orders, including identical keys.
    for (int a = 0; a < kBoundActionCount; ++a) for (int z = 0; z < kBoundActionCount; ++z) for (int order = 0; order < 2; ++order) {
        s.Key(c.device[0], b.key[0][a], true); s.Key(c.device[1], b.key[0][z], true);
        Check(buttons(0) == kBoundActions[a].bit && buttons(1) == kBoundActions[z].bit, "simultaneous independent actions");
        s.Key(c.device[order], b.key[0][order ? z : a], false);
        Check(buttons(order) == 0, "released player's action cleared");
        Check(buttons(1-order) == kBoundActions[order ? a : z].bit, "other player's key remains held");
        s.Key(c.device[1-order], b.key[0][order ? a : z], false);
        Check(buttons(0) == 0 && buttons(1) == 0, "no stuck release in either order"); s.TakePresses();
    }
    // Exhaust all sets of the six attack buttons on both keyboards (4096 pairs).
    for (unsigned a = 0; a < 64; ++a) for (unsigned z = 0; z < 64; ++z) {
        for (int k = 0; k < 6; ++k) { s.Key(c.device[0], b.key[0][k+4], (a>>k)&1); s.Key(c.device[1], b.key[0][k+4], (z>>k)&1); }
        Check(buttons(0) == a<<4 && buttons(1) == z<<4, "twelve attack keys without cross-player suppression"); s.TakePresses();
    }
    for (int p = 0; p < 2; ++p) for (int k = 0; k < 6; ++k) s.Key(c.device[p], b.key[0][k+4], false);
    InputProfile profile; profile.players[0].neutralize = profile.players[1].neutralize = true;
    InputProcessor processor;
    s.Key(c.device[0], SDL_SCANCODE_A, true); s.Key(c.device[1], SDL_SCANCODE_D, true);
    Check(processor.Apply(0, buttons(0), 1, profile) == 2 && processor.Apply(1, buttons(1), 1, profile) == 1,
          "opposite directions on different players never cancel");
    s.Key(c.device[0], SDL_SCANCODE_D, true); s.Key(c.device[0], SDL_SCANCODE_S, true);
    Check(processor.Apply(0, buttons(0), 2, profile) == 4 && processor.Apply(1, buttons(1), 2, profile) == 1,
          "SOCD applies only within each player");
    const auto saved = c;
    s.Disconnect(c.device[0]); Check(buttons(0) == 0 && buttons(1) == 1, "unplug clears only its player");
    s.Connect("new-device", "Identical keyboard"); s.Key("new-device", SDL_SCANCODE_T, true);
    Check(buttons(0) == 0 && buttons(1) == 1, "different keyboard never takes an absent player's place");
    s.Connect(c.device[0], "Identical keyboard"); s.Key(c.device[0], SDL_SCANCODE_T, true);
    Check(buttons(0) == 16 && buttons(1) == 1, "stable reconnect restores assignment without stale keys");
    s.Focus(false); Check(buttons(0) == 0 && buttons(1) == 0 && s.TakePresses().empty(), "focus loss clears gameplay and shortcuts");
    s.Focus(true); s.Key(c.device[0], SDL_SCANCODE_T, true);
    Check(buttons(0) == 0, "held key cannot reappear on focus regain");
    s.Key(c.device[0], SDL_SCANCODE_T, false); s.Key(c.device[0], SDL_SCANCODE_T, true);
    Check(buttons(0) == 16, "release then fresh press works after refocus");
    auto bad = c; bad.device[1] = bad.device[0];
    Check(!s.ForPlayer(bad, 1)[SDL_SCANCODE_T] && s.ForPlayer(bad, 0)[SDL_SCANCODE_T], "duplicate corrupted assignment fails closed");
    Check(c.device == saved.device && c.enabled == saved.enabled && c.mirrorP1 == saved.mirrorP1, "hotplug and focus never mutate settings");
}
void Shortcuts() {
    const auto c = Config(); auto b = SameKeys(); auto hotkeys = DefaultHotkeys();
    BindHotkey(hotkeys, static_cast<int>(HotkeyAction::Save), {SDL_SCANCODE_T, 1});
    KeyboardState s; s.Focus(true);
    s.Key(c.device[0], SDL_SCANCODE_LCTRL, true); s.Key(c.device[1], SDL_SCANCODE_T, true);
    const auto &p1 = s.ForPlayer(c,0), &p2 = s.ForPlayer(c,1);
    Check(HotkeyModifiers(s.Modifiers(p1)) == 1 && HotkeyModifiers(s.Modifiers(p2)) == 0, "modifiers belong to their physical device");
    for (const auto &press : s.TakePresses()) if (press.key == SDL_SCANCODE_T)
        Check(!KeyboardHotkeyAllowed(c,press), "P2 cannot run P1 shortcuts");
    Check(ReadKeyboardButtons(b,p2.data(),0,hotkeys,s.Modifiers(p2),false)==16, "P2 attack works while P1 holds Ctrl");
    s.Key(c.device[0], SDL_SCANCODE_T, true);
    auto press = s.TakePresses().back();
    Check(KeyboardHotkeyAllowed(c,press) && FindHotkey(hotkeys,press.key,press.mods,press.repeat)==HotkeyAction::Save,
          "P1 chord uses its own modifiers");
    Check(ReadKeyboardButtons(b,p1.data(),0,hotkeys,s.Modifiers(p1))==0, "P1 shortcut does not also punch");
    s.Key(c.device[0], SDL_SCANCODE_T, true); press=s.TakePresses().back();
    Check(press.repeat && FindHotkey(hotkeys,press.key,press.mods,press.repeat)==HotkeyAction::None, "typematic does not repeat save/reset");
    for (const auto &id : {c.device[0],c.device[1],std::string("unassigned")})
        Check(KeyboardHotkeyAllowed(c,{id,SDL_SCANCODE_ESCAPE,SDL_KMOD_NONE,false}) &&
            FindHotkey(hotkeys,SDL_SCANCODE_ESCAPE,SDL_KMOD_NONE)==HotkeyAction::Menu, "fixed Escape always available");
    b.key[0][4]=SDL_SCANCODE_P;
    KeyboardState::Keys keys{}; keys[SDL_SCANCODE_P]=true;
    Check(ReadKeyboardButtons(b,keys.data(),0,hotkeys,SDL_KMOD_NONE,true)==0 &&
          ReadKeyboardButtons(b,keys.data(),0,hotkeys,SDL_KMOD_NONE,false)==16, "only P1 reserves global shortcut keys");
}
void AltDoesNotMuteControls() {
    auto config = Config();config.mirrorP1 = false;
    auto bindings = SameKeys();auto hotkeys = DefaultHotkeys();
    KeyboardState state;state.Focus(true);
    const auto read = [&](int player) { return ReadKeyboardPlayerButtons(state,config,player,bindings,hotkeys); };
    for (int player = 0; player < 2; ++player) for (const auto alt : {SDL_SCANCODE_LALT,SDL_SCANCODE_RALT}) {
        const auto &id = config.device[player];
        for (const auto key : {SDL_SCANCODE_S,SDL_SCANCODE_D,SDL_SCANCODE_U,SDL_SCANCODE_I,SDL_SCANCODE_R}) state.Key(id,key,true);
        const auto before = read(player);
        state.Key(id,alt,true);
        Check(before == (4u|1u|64u|4096u|16384u) && read(player)==before,"Alt does not silence maintained directions, attack or macros");
        Check(read(1-player)==0,"Alt cannot activate the other player's controls");
        state.Key(id,SDL_SCANCODE_RETURN,true);
        Check(read(player)==(before | (player==1?1024u:0u)),"Alt+Enter reserves only P1 shortcut key and preserves unrelated controls");
        state.Key(id,SDL_SCANCODE_RETURN,false);state.Key(id,alt,false);
        Check(read(player)==before,"Alt release does not require releasing and repressing maintained controls");
        for (const auto key : {SDL_SCANCODE_S,SDL_SCANCODE_D,SDL_SCANCODE_U,SDL_SCANCODE_I,SDL_SCANCODE_R}) state.Key(id,key,false);
    }
    BindHotkey(hotkeys,static_cast<int>(HotkeyAction::Save),{SDL_SCANCODE_U,4});
    state.Key(config.device[0],SDL_SCANCODE_RALT,true);state.Key(config.device[0],SDL_SCANCODE_D,true);state.Key(config.device[0],SDL_SCANCODE_U,true);
    state.Key(config.device[1],SDL_SCANCODE_D,true);state.Key(config.device[1],SDL_SCANCODE_U,true);
    Check(read(0)==1u && read(1)==65u,"configured Alt attack shortcut consumes only P1 attack, never held direction or P2 attack");
    for (const auto &id : config.device) {state.Key(id,SDL_SCANCODE_RALT,false);state.Key(id,SDL_SCANCODE_D,false);state.Key(id,SDL_SCANCODE_U,false);}
    // Replay the exact raw masks seen with transient Right Alt in both captures.
    for (int player=0;player<2;++player) {
        hotkeys=DefaultHotkeys();
        for(const auto key:{SDL_SCANCODE_D,SDL_SCANCODE_U,SDL_SCANCODE_I})state.Key(config.device[player],key,true);
        state.Key(config.device[player],SDL_SCANCODE_RALT,true);
        Check(read(player)==4161u,"captured D+U+I+Right Alt retains movement and x3 punches");
        state.Key(config.device[player],SDL_SCANCODE_RALT,false);
        for(const auto key:{SDL_SCANCODE_D,SDL_SCANCODE_U,SDL_SCANCODE_I})state.Key(config.device[player],key,false);
    }
}
void ReplayCapture(const char *path) {
    std::ifstream in(path);Check(in.good(),"captured input replay fixture readable");
    KeyboardState state;auto config=Config();Bindings bindings{};Hotkeys hotkeys{};
    std::string line;int samples=0,packets=0;
    while(std::getline(in,line)) {
        std::istringstream fields(line);char op=0;fields>>op;
        if(op=='C') {std::string id,group;fields>>id>>group;state.Connect(id,"Replay keyboard",group);}
        else if(op=='A') {int mirror=0;fields>>config.device[0]>>config.device[1]>>mirror;config.mirrorP1=mirror!=0;}
        else if(op=='B') {int player=0,action=0,key=0;fields>>player>>action>>key;bindings.key[player][action]=static_cast<SDL_Scancode>(key);}
        else if(op=='H') {int action=0,key=0;unsigned mods=0;fields>>action>>key>>mods;hotkeys[action]={static_cast<SDL_Scancode>(key),mods};}
        else if(op=='F') {int focused=0;fields>>focused;state.Focus(focused!=0);}
        else if(op=='K') {std::string id;int key=0,down=0;fields>>id>>key>>down;state.Key(id,static_cast<SDL_Scancode>(key),down!=0);++packets;}
        else if(op=='S') {
            int player=0;uint32_t expected=0;uint64_t sequence=0;fields>>player>>expected>>sequence;
            const auto actual=ReadKeyboardPlayerButtons(state,config,player,bindings,hotkeys);
            if(actual!=expected)std::fprintf(stderr,"Replay sequence %llu player %d: %x != %x\n",static_cast<unsigned long long>(sequence),player+1,actual,expected);
            Check(actual==expected,"production player sampling matches captured held keys and per-key shortcut reservations");
            ++samples;state.TakePresses();
        }
        Check(!fields.fail(),"captured input replay record valid");
    }
    std::printf("Capture replay: %d raw packets, %d player samples checked. Recorded events, not a new physical session.\n",packets,samples);
}
void SettingsAndActions() {
    const auto folder = std::filesystem::temp_directory_path() / ("cps2-keyboard-tests-" + std::to_string(SDL_GetTicksNS()));
    std::filesystem::create_directory(folder);
    const auto file = folder/"keyboards.ini", game = folder/"settings.ini";
    { std::ofstream out(game); out << "[keyboard1]\nup=W\nlp=T\n[features]\npreset=pure\n[unknown]\nkeep=yes\n"; }
    const auto before = Contents(game);
    auto config=LoadKeyboards(file.string());
    Check(!config.enabled && config.mirrorP1 && !std::filesystem::exists(file), "default leaves existing input and files untouched");
    { std::ofstream out(file); out << "[future]\nkeep=value\n"; }
    config=Config(); Check(SaveKeyboards(file.string(),config), "save global device assignments");
    const auto loaded=LoadKeyboards(file.string());
    Check(loaded.device==config.device && loaded.enabled && loaded.mirrorP1 && Contents(file).find("keep=value")!=std::string::npos,
          "roundtrip preserves IDs and unknown settings");
    const auto good=Contents(file); config.device[1]=config.device[0];
    Check(!SaveKeyboards(file.string(),config) && Contents(file)==good, "duplicate assignment cannot overwrite valid settings");
    auto b=SameKeys(); const auto original=b;
    BindKeyboardKey(b,BindingTable(),1,4,SDL_SCANCODE_W);
    Check(b.key[0][0]==SDL_SCANCODE_W && b.key[1][0]==SDL_SCANCODE_T && b.key[1][4]==SDL_SCANCODE_W,
          "P2 rebind swaps within P2 without deleting P1 keys");
    b=original; BindKeyboardKey(b,BindingTable(),0,4,SDL_SCANCODE_W);
    Check(b.key[1][0]==SDL_SCANCODE_W, "rebinding P1 never deletes P2, even with separate mode off");
    // The reported build_007 failure: P1 W erased when assigning P2 W with
    // separate=0, regardless of mirror_p1. Persist and reload every layout.
    for (bool enabled : {false,true}) for (bool mirror : {false,true}) {
        auto choices=Config(); choices.enabled=enabled; choices.mirrorP1=mirror;
        Check(SaveKeyboards(file.string(),choices), "save every mode/mirror combination");
        b=DefaultBindings();
        for (int player : {0,1,0,1}) for (int a=0;a<kBoundActionCount;++a) {
            BindKeyboardKey(b,BindingTable(),player,a,original.key[player][a]);
            const int other=1-player;
            if (b.key[other][a]!=SDL_SCANCODE_UNKNOWN)
                Check(b.key[other][a]==original.key[other][a], "same layout preserved on other player in all toggle states");
        }
        recomp::Ini ini(game.string()); ini.load();
        for (int player=0;player<2;++player) for(int a=0;a<kBoundActionCount;++a)
            ini.set(recomp::BindingSection(false,player),kBoundActions[a].name,recomp::KeyText(b.key[player][a]));
        Check(ini.save(), "persist independent same-key layouts");
        const auto reloaded=LoadBindings(game.string());
        for (int player=0;player<2;++player) for(int a=0;a<kBoundActionCount;++a)
            Check(reloaded.key[player][a]==original.key[player][a], "both full layouts survive disk reload");
    }
    {std::ofstream out(game,std::ios::binary);out<<before;}
    InputProfile saved; saved.players[0].neutralize=true; saved.players[0].custom[0]=5;
    saved.players[0].autofire[0]=true; saved.players[1].custom[0]=42;
    const auto effective=KeyboardActionOptions(loaded,saved);
    Check(effective.players[1].custom[0]==5 && effective.players[1].neutralize && saved.players[1].custom[0]==42,
          "P2 inherits action options without overwriting saved P2");
    auto off=loaded; off.enabled=false;
    Check(KeyboardActionOptions(off,saved).players[1].custom[0]==42, "disabling separate mode restores original P2 options");
    InputProcessor processor;
    Check(processor.Apply(0,16,100,effective)==16 && processor.Apply(1,0,100,effective)==0, "P1 autofire starts independently");
    Check(processor.Apply(0,16,101,effective)==0 && processor.Apply(1,16,101,effective)==16, "P2 autofire has an independent phase");
    Check(processor.Apply(0,0,102,effective)==0 && processor.Apply(1,16,102,effective)==0, "P1 release does not reset P2 autofire");
    Check(processor.Apply(0,1u<<14,103,effective)==80 && processor.Apply(1,1u<<14,103,effective)==80,
          "custom combination applies to both players");
    Check(Contents(game)==before, "device operations never write game bindings or features");
    std::filesystem::remove_all(folder);
}
void AssignmentAndCapture() {
    KeyboardConfig c;
    Check(AssignKeyboardDevice(c,0,"physical-A") && !c.enabled, "first assignment keeps default mode until second keyboard");
    Check(!AssignKeyboardDevice(c,1,"physical-A") && c.device[1].empty(), "cannot assign one physical keyboard to both players");
    Check(AssignKeyboardDevice(c,1,"physical-B") && c.enabled, "assigning second keyboard automatically enables separation");
    c.mirrorP1=false;
    for(int p=0;p<2;++p) {
        const std::vector<KeyboardPress> wrong={{c.device[1-p],SDL_SCANCODE_W,SDL_KMOD_NONE,false},
            {c.device[1-p],SDL_SCANCODE_BACKSPACE,SDL_KMOD_NONE,false}};
        Check(KeyboardBindingKey(c,p,wrong)==SDL_SCANCODE_UNKNOWN, "wrong physical keyboard cannot bind or clear selected player's action");
        auto right=wrong;right.push_back({c.device[p],SDL_SCANCODE_W,SDL_KMOD_NONE,false});
        Check(KeyboardBindingKey(c,p,right)==SDL_SCANCODE_W, "assigned physical keyboard binds W independently");
        Check(KeyboardBindingKey(c,p,{{c.device[p],SDL_SCANCODE_W,SDL_KMOD_NONE,true}})==SDL_SCANCODE_UNKNOWN,
              "held key repeat does not prematurely bind");
        Check(KeyboardBindingKey(c,p,{{c.device[p],SDL_SCANCODE_BACKSPACE,SDL_KMOD_NONE,false}})==SDL_SCANCODE_BACKSPACE,
              "only assigned keyboard can clear binding");
        Check(KeyboardBindingKey(c,p,{{c.device[1-p],SDL_SCANCODE_ESCAPE,SDL_KMOD_NONE,false}})==SDL_SCANCODE_ESCAPE,
              "Escape from either device cancels capture");
    }
    c.enabled=false;
    Check(KeyboardBindingKey(c,0,{{c.device[0],SDL_SCANCODE_W,SDL_KMOD_NONE,false}})==SDL_SCANCODE_UNKNOWN,
          "physical capture inactive in explicitly shared mode");
}
void HeldAndTapStress() {
    const auto c=Config();const auto b=SameKeys();const Hotkeys hotkeys{};
    KeyboardState s;s.Focus(true);InputProcessor processor;
    InputProfile profile;profile.players[0].neutralize=profile.players[1].neutralize=true;
    profile.players[0].custom[0]=profile.players[1].custom[0]=5;
    // Keep the reported diagonals/attacks held on one keyboard while the other
    // rapidly taps the identical keys, chords, and macros in alternating order.
    const std::array<std::array<SDL_Scancode,3>,2> chords={{{SDL_SCANCODE_S,SDL_SCANCODE_D,SDL_SCANCODE_U},
        {SDL_SCANCODE_A,SDL_SCANCODE_W,SDL_SCANCODE_J}}};
    for(int held=0;held<2;++held)for(const auto &chord:chords) {
        uint16_t expected=0;
        for(auto key:chord) {s.Key(c.device[held],key,true);for(int a=0;a<kBoundActionCount;++a)if(b.key[0][a]==key)expected|=kBoundActions[a].bit;}
        for(uint64_t frame=0;frame<5000;++frame) {
            const bool down=(frame%3)!=0;
            for(auto key:chord)s.Key(c.device[1-held],key,down);
            s.Key(c.device[1-held],SDL_SCANCODE_R,frame%5==0);
            s.Key(c.device[1-held],SDL_SCANCODE_I,frame%7==0);
            s.Key(c.device[1-held],SDL_SCANCODE_K,frame%11==0);
            for(int p=0;p<2;++p) {
                const auto &keys=s.ForPlayer(c,p);
                const auto raw=ReadKeyboardButtons(b,keys.data(),0,hotkeys,s.Modifiers(keys),false);
                const auto output=processor.Apply(p,raw,frame,profile);
                if(p==held)Check(output==expected,"other keyboard's taps/repeats/macros cannot interrupt held S+D+U or A+W+J");
            }
            s.TakePresses();
        }
        for(int p=0;p<2;++p)for(int a=0;a<kBoundActionCount;++a)s.Key(c.device[p],b.key[0][a],false);
    }
}
void CompositeKeyboards() {
    KeyboardState s;auto c=Config();c.device={"A-main","B-main"};s.Focus(true);
    // Same labels/models do not imply the same physical device.
    s.Connect("A-main","Same model","container-A");s.Connect("A-extra","Same model","container-A");
    s.Connect("B-main","Same model","container-B");s.Connect("B-extra","Same model","container-B");
    const auto resolved=s.ResolveConfig(c);
    s.Key("A-extra",SDL_SCANCODE_W,true);
    const auto captured=s.TakePresses();
    Check(KeyboardBindingKey(resolved,0,captured)==SDL_SCANCODE_W,"binding accepts assigned keyboard's auxiliary collection");
    Check(KeyboardBindingKey(resolved,1,captured)==SDL_SCANCODE_UNKNOWN,"binding rejects other physical keyboard's auxiliary collection");
    s.Key("A-extra",SDL_SCANCODE_W,false);
    KeyboardState delayed;const auto pending=delayed.ResolveConfig(c);
    delayed.Connect("A-main","Keyboard","container-A");
    Check(delayed.ResolveConfig(pending).device[0]=="container-A","legacy assignment resolves after delayed connection");
    Check(resolved.device[0]=="container-A" && resolved.device[1]=="container-B","resolve legacy assignments to distinct physical keyboards");
    Check(s.Interfaces(c.device[0])==2 && s.Interfaces(c.device[1])==2,"both HID collections belong to their own keyboard");
    s.Key("A-main",SDL_SCANCODE_S,true);s.Key("A-extra",SDL_SCANCODE_D,true);s.Key("A-extra",SDL_SCANCODE_U,true);
    auto snapshot=s.Snapshot(c,0);
    Check(snapshot.keys[SDL_SCANCODE_S] && snapshot.keys[SDL_SCANCODE_D] && snapshot.keys[SDL_SCANCODE_U],"chord can span primary and auxiliary interfaces");
    Check(!s.ForPlayer(c,1)[SDL_SCANCODE_S],"auxiliary interface never leaks to another physical keyboard");
    for(auto key:{SDL_SCANCODE_S,SDL_SCANCODE_D,SDL_SCANCODE_U})s.Key("B-extra",key,true);
    Check(s.ForPlayer(c,1)[SDL_SCANCODE_S] && s.ForPlayer(c,1)[SDL_SCANCODE_U],"second keyboard accepts all keys from its auxiliary interface");
    s.Key("A-extra",SDL_SCANCODE_S,true);s.TakePresses();
    s.Key("A-main",SDL_SCANCODE_S,false);
    Check(s.ForPlayer(c,0)[SDL_SCANCODE_S] && s.ForPlayer(c,1)[SDL_SCANCODE_S],"one interface release cannot clear a key held on its sibling or another player");
    s.Key("A-extra",SDL_SCANCODE_S,false);
    Check(!s.ForPlayer(c,0)[SDL_SCANCODE_S] && s.ForPlayer(c,1)[SDL_SCANCODE_S],"last sibling release clears only its physical keyboard");
    s.Disconnect("A-main");
    Check(s.ForPlayer(c,0)[SDL_SCANCODE_U] && s.Interfaces(c.device[0])==1,"legacy assignment survives primary interface disconnect while sibling remains");
    Check(s.PlayerFor(c,"A-extra")==1 && s.PlayerFor(c,"B-extra")==2,"diagnostics identify both auxiliary players");
    s.Connect("A-main","Same model","container-A");s.Key("A-main",SDL_SCANCODE_LCTRL,true);s.Key("A-extra",SDL_SCANCODE_T,true);
    auto presses=s.TakePresses();auto hot=DefaultHotkeys();BindHotkey(hot,static_cast<int>(HotkeyAction::Save),{SDL_SCANCODE_T,1});
    bool chord=false;
    for(const auto &press:presses)if(press.key==SDL_SCANCODE_T)chord=press.device=="container-A" && FindHotkey(hot,press.key,press.mods,press.repeat)==HotkeyAction::Save;
    Check(chord,"modifier and attack combine across interfaces of one physical keyboard only");
    s.Key("A-main",SDL_SCANCODE_T,true);presses=s.TakePresses();
    Check(presses.size()==1 && presses[0].repeat,"duplicate reports do not repeat a physical keyboard shortcut");
    s.Key("A-main",SDL_SCANCODE_W,true);s.Key("A-extra",SDL_SCANCODE_J,true);s.Disconnect("A-main");
    presses=s.TakePresses();
    Check(presses.size()==1 && presses[0].key==SDL_SCANCODE_J,"disconnect discards only that interface's queued presses");
    auto invalid=c;invalid.device[1]="A-extra";
    Check(!s.Snapshot(invalid,1).connected,"assigning two interfaces of one keyboard cannot create two players");
    Check(s.ResolveConfig(invalid).device[0]==s.ResolveConfig(invalid).device[1],"UI rejects assigning a sibling collection as another keyboard");
    s.Focus(false);s.Focus(true);s.Connect("A-new","Same model","container-A");s.Key("A-new",SDL_SCANCODE_J,true);
    Check(!s.ForPlayer(c,0)[SDL_SCANCODE_J],"new sibling cannot bypass focus-loss release guard");
    // Stress duplicate reports and alternating sibling taps while another board
    // holds the exact same chord. Old per-interface routing drops the extra path.
    KeyboardState stress;stress.Focus(true);
    for(const auto &id:{"A-main","A-extra","B-main","B-extra"})stress.Connect(id,"Keyboard",id[0]=='A'?"container-A":"container-B");
    for(auto key:{SDL_SCANCODE_S,SDL_SCANCODE_D,SDL_SCANCODE_U})stress.Key("A-main",key,true);
    for(int i=0;i<12000;++i) {
        const auto id=i%2?"B-main":"B-extra";
        for(auto key:{SDL_SCANCODE_S,SDL_SCANCODE_D,SDL_SCANCODE_U})stress.Key(id,key,i%3!=0);
        const auto &one=stress.ForPlayer(c,0);
        Check(one[SDL_SCANCODE_S] && one[SDL_SCANCODE_D] && one[SDL_SCANCODE_U],"composite keyboard stress cannot interrupt other player's maintained chord");
        const auto two=stress.Snapshot(c,1);const auto &a=stress.Devices().at("B-main");const auto &b=stress.Devices().at("B-extra");
        Check(two.keys[SDL_SCANCODE_U]==(a.keys[SDL_SCANCODE_U]||b.keys[SDL_SCANCODE_U]),"sibling reference state remains correct under repeated make/break");
        stress.TakePresses();
    }
}
void DiagnosticFixture() {
    const auto c=Config();const auto b=SameKeys();KeyboardState s;s.Focus(true);
    InputTraceConfigure(c);InputTraceFocus(true);InputTraceFrame(12);
    const auto send=[&](int player,SDL_Scancode key,unsigned make,bool down) {
        s.Key(c.device[player],key,down);
        InputTraceRaw(s,c.device[player],key,make,down?0:1,0,down,false,100);
        for(int p=0;p<2;++p) {
            const auto &keys=s.ForPlayer(c,p);
            const auto mask=ReadKeyboardButtons(b,keys.data(),0,Hotkeys{},s.Modifiers(keys),false);
            InputTracePlayer(s,c,p,mask,0,0,static_cast<uint16_t>(mask),false,false);
        }
    };
    send(0,SDL_SCANCODE_S,0x1F,true);send(0,SDL_SCANCODE_D,0x20,true);send(0,SDL_SCANCODE_U,0x16,true);
    send(1,SDL_SCANCODE_S,0x1F,true);send(1,SDL_SCANCODE_D,0x20,true);send(1,SDL_SCANCODE_U,0x16,true);
    send(1,SDL_SCANCODE_S,0x1F,false);send(1,SDL_SCANCODE_D,0x20,false);send(1,SDL_SCANCODE_U,0x16,false);
    s.Focus(false);InputTraceFocus(false);
    send(1,SDL_SCANCODE_X,0x2D,true); // Must be redacted while in the background.
    s.Focus(true);InputTraceFocus(true);
    InputTracePlayer(s,c,0,0,0,0,0,false,false);
    InputTraceNote("SYNTHETIC_FIXTURE","Test input only; this is not a physical keyboard capture.");
}
void Decode() {
    for (unsigned flags : {0u,1u}) {
        Check(RawKeyboardScancode(0x11,flags,0x57)==SDL_SCANCODE_W, "physical W make and break");
        Check(RawKeyboardScancode(0x48,flags,0x68)==SDL_SCANCODE_KP_8, "keypad differs from extended arrows");
        Check(RawKeyboardScancode(0x48,flags|2,0x26)==SDL_SCANCODE_UP, "extended Up make and break");
        Check(RawKeyboardScancode(0x1D,flags|2,0x11)==SDL_SCANCODE_RCTRL, "right Ctrl retains device-local modifier");
    }
    Check(RawKeyboardScancode(0x1D,4,0x11)==SDL_SCANCODE_UNKNOWN && RawKeyboardScancode(0x45,4,0x13)==SDL_SCANCODE_PAUSE,
          "Pause sequence does not leave phantom Ctrl");
    Check(RawKeyboardScancode(0x1E,0,255)==SDL_SCANCODE_UNKNOWN, "ignore keyboard overrun packets");
}
void Native() {
#ifdef _WIN32
    SDL_SetHintWithPriority(SDL_HINT_WINDOWS_RAW_KEYBOARD,"0",SDL_HINT_OVERRIDE);
    Check(SDL_Init(SDL_INIT_VIDEO), "native SDL initialization");
    auto *window=SDL_CreateWindow("Keyboard receiver test",100,100,SDL_WINDOW_HIDDEN);
    Check(window!=nullptr, "hidden Windows owner");
    {
        RawKeyboards raw; Check(raw.Open(window), "native Raw Input registration");
        UINT count=0; GetRegisteredRawInputDevices(nullptr,&count,sizeof(RAWINPUTDEVICE));
        std::vector<RAWINPUTDEVICE> registrations(count);
        const UINT found=GetRegisteredRawInputDevices(registrations.data(),&count,sizeof(RAWINPUTDEVICE));
        bool registered=false;
        if(found!=UINT(-1)) for(UINT i=0;i<found;++i) if(registrations[i].usUsagePage==1 && registrations[i].usUsage==6) {
            wchar_t name[128]{}; GetClassNameW(registrations[i].hwndTarget,name,128);
            std::printf("Raw registration: flags=%08lx class=%ls\n",registrations[i].dwFlags,name);
            // Windows omits DEVNOTIFY from this readback on some versions.
            registered=std::wstring(name)==L"CPS2InputLabKeyboards" && (registrations[i].dwFlags&RIDEV_INPUTSINK) &&
                       !(registrations[i].dwFlags&RIDEV_NOLEGACY);
        }
        std::printf("Registration count=%u found=%u error=%lu\n",count,found,GetLastError());
        Check(registered, "independent native sink registered without disabling legacy keys");
        SDL_Event event; while(SDL_PollEvent(&event)) {} raw.PollFocus();
        std::printf("Native receiver OK; Windows keyboard interfaces enumerated: %zu\n",raw.State().Devices().size());
        std::map<std::string,int> groups;
        for(const auto &[id,d]:raw.State().Devices())++groups[d.physicalID];
        std::printf("Native physical keyboard groups: %zu\n",groups.size());
        for(const auto &[id,d]:raw.State().Devices())Check(raw.State().Interfaces(id)==groups[d.physicalID],"native Windows physical-container grouping consistent");
        for(const auto &[id,d]:raw.State().Devices()) Check(id.rfind("winraw:",0)==0 && d.connected, "persistent Windows device identity");
        Check(raw.State().TakePresses().empty(), "hidden receiver cannot issue background shortcuts");
        SDL_Window *replacement = SDL_CreateWindow("Replacement shader window", 320, 240, SDL_WINDOW_HIDDEN);
        Check(replacement != nullptr, "shader replacement window exists");
        const auto devicesBefore = raw.State().Devices().size();
        Check(raw.Retarget(replacement), "existing input sink retargets after a renderer/window change");
        Check(raw.State().Devices().size() == devicesBefore, "renderer change keeps physical keyboard/interface identities");
        Check(!raw.State().Focused(), "hidden replacement cannot receive background game controls");
        SDL_DestroyWindow(replacement);
    }
    SDL_DestroyWindow(window); SDL_Quit();
#endif
}
}
int main(int argc,char **argv) {
    Routing(); Shortcuts(); AltDoesNotMuteControls(); SettingsAndActions(); AssignmentAndCapture(); HeldAndTapStress(); CompositeKeyboards(); Decode();
    if(argc>2 && std::string(argv[1])=="--replay-capture") ReplayCapture(argv[2]);
    if(argc>1 && std::string(argv[1])=="--native") Native();
    if(argc>1 && std::string(argv[1])=="--trace-fixture") DiagnosticFixture();
    std::printf("PASS: %d separate-keyboard checks. Synthetic two-device routing; physical dual-keyboard play requires user validation.\n",checks);
}
