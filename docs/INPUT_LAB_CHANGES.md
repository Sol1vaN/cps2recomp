# Input Lab 018: functional differences

English | [Español](INPUT_LAB_CHANGES.es.md)

Comparison base: upstream 1.2.0, [`f1212bca`](https://github.com/tmcleroy/cps2recomp/commit/f1212bca038f543e30fb106d8f88dabf30374df5).
The [upstream inventory](UPSTREAM_DIFFERENCES.md) separately records later official improvements.
See also the [feature gallery](SCREENSHOTS.md) and [complete cheat guide](CHEATS.md).

## Inherited work

The eighteen games, 68000/Z80 translators, board model, GPU/SDL renderer, shaders, widescreen, zoom,
training/alternative modes, colour editor, soundtracks, Multi game, lobby, chat and rollback are upstream work.
Button configuration, shared/own layouts, hidden-character features and rapid fire already existed.
Input Lab extends these systems; it does not claim authorship of them.

## RawInput Multi Keyboard

Windows RawInput keeps pressed, released, blocked and connected state independently for every interface.
States are ORed only inside the same physical keyboard group. Releasing W on P1 cannot release W on P2;
releasing W on one auxiliary interface cannot erase W held by another member of that keyboard's group.
Two friends can therefore use the same layout on two keyboards without cross-device software ghosting.

Physical identity uses the Windows PnP container. A matching name, VID/PID or model alone is insufficient.
Null/system containers are not evidence of identity; uncertain interfaces remain separate. No development-machine
interface numbers are hardcoded. A mouse exposing keyboard interfaces is not merged merely because it emits the same keys.

Controllers → Use separate keyboards assigns a physical keyboard to each player and displays Keys held.
Assignments survive reconnects and restarts; connecting a device does not rewrite bindings. P2 uses Player 1's
layout borrows keys/options without erasing P2's saved layout. Binding the same key for P2 no longer clears P1's key.
In separate mode global shortcuts belong to P1; Escape can open the menu from either keyboard.

Windows only. This fixes software state mixing, not electrical keyboard rollover limits. Normal play requires
no logger; observation delays in an optional trace are not automatically evidence of lost inputs.
Sources: [keyboards](../port/keyboards.h), [RawInput](../port/raw_keyboards.h), [launcher UI](../launcher/src/keyboard_controls.cpp).

## Neutralize Opposing Directions

Optional per game and player: Left+Right cancel horizontally; Up+Down cancel vertically. Left+Down+Right
retains Down. Cancellation applies to the merged input state, without replacing one direction with its opposite.
This follows MAME-style neutral semantics and made keyboard motions and combos feel comfortable in the user's playtests;
no measured latency equivalence with MAME is claimed.

Related per-game controls: PPP/KKK, four Custom masks of simultaneous attack buttons, and autofire per action
(six attacks, PPP, KKK and four Custom actions). Custom actions are not timed motion scripts. Autofire uses
1–30 on/off game frames, starts immediately, resets when released and does not advance twice for speculative run-ahead.
Upstream rapid fire is folded into this single processor rather than running a duplicate implementation.
Other games keep their own masks, autofire and direction choice even when bindings are shared.
Sources: [input profile](../port/input_profile.h), [launcher UI](../launcher/src/input_controls.cpp), [menu](../port/menu.cpp).

## Physics Assignment Joysticks

The requested name refers to **physical gamepad assignment**, not altered character physics.
Physical assignment is independent of SDL enumeration order. A single gamepad can be reserved for P2 while
P1 uses a keyboard; automatic P1 cannot consume that reserved device. Buttons, triggers and both sticks'
signed directions can be bound and cleared. Shared/own game layouts remain available.
Source: [assignment resolver](../port/gamepad_assignment.h).

## Global Bindable Hotkeys

While playing keyboard shortcuts are global and support modifiers. Escape remains fixed. Reset game has its
own action, replacing the old alternate menu row. Pause, frame advance, speed, training restart, states/slots,
test/service, fullscreen, chat and zoom can be edited without changing the game's bindings.

Gamepad — Global shortcuts makes Guide, left stick click and Start+Select independently editable/clearable.
Pause/chat, frame advance, training, zoom and Multi actions are also configurable. Up to three inputs may
form a chord, including triggers and stick directions. Clear all gamepad shortcuts and Restore gamepad shortcuts
affect only pad shortcuts. An empty binding persists as `none` and does not silently return after restart.
Conflicts are resolved within the same context; offline pause and online chat may share an input.
Chords have priority, while individual buttons may retain game actions. When menu and camera share a button,
a tap opens the menu; camera movement cancels that tap.
Sources: [keyboard shortcuts](../port/hotkeys.h), [pad shortcuts](../port/pad_hotkeys.h), [UI](../launcher/src/global_controls.cpp).

## Audio Equalizer

Game → Sound → Equalizer contains five parametric bands, adapted from Olivier Galibert's MAMEPPK/MAME DSP
under its retained [BSD-3-Clause notice](../licenses/mame-equalizer.txt).

| Band | Default | Frequency range | Mode |
|---|---:|---:|---|
| Low | 100 Hz | 20–2000 Hz | Shelf / Peak |
| Low Mid | 330 Hz | 100–10000 Hz | Peak |
| Mid | 1000 Hz | 100–10000 Hz | Peak |
| High Mid | 3300 Hz | 100–10000 Hz | Peak |
| High | 10000 Hz | 500–16000 Hz | Shelf / Peak |

Gain: −12 to +12 dB in 0.1 dB steps. Peak Q: 0.10–10.00, default 0.7071067.
Active/Bypass; initially flat at 0 dB. Left/Right adjusts, Shift makes larger steps, Enter restores a slider,
Reset All resets only this game's EQ. The menu remains paused; listen after resuming.
Board audio and custom music are processed in floating point with independent channel/source state and
Nyquist limits. Filters reset on state load, game reset, parameter or source-rate changes. EQ is outside
game states/hashes; loading a state does not overwrite the chosen EQ. Values live in each game's `[equalizer]` settings.
Source: [equalizer](../port/equalizer.h).

## Sliders

Slider controls stores six values independently per game, inspired by MAMEPPK:

| Control | Range | Meaning |
|---|---|---|
| Overclock CPU :maincpu | 25–400%, step 0.1 | Main CPU work available between video interrupts. |
| Overclock CPU :audiocpu | 25–400%, step 0.1 | Audio CPU clock; separate from QSound and sample rate. |
| Screen Refresh Rate | 40–90 Hz, step 0.1 | Virtual board video frequency, not a monitor mode switch. |
| Screen Brightness / Contrast | 0–2, step 0.01 | Local image transformation. |
| Screen Gamma | 0.1–3, step 0.01 | Local midtone correction. |

The native CPS2 default is `8000000 / 512 / 262` Hz. Refresh, CPU clock and emulation speed are distinct;
their gameplay effects depend on the game. Definitive's extra audio retains its own timing. Older driver-based
save states may require Reset game for audio-clock changes. Online uses native clocks and preserves offline
values. Picture adjustments leave native video RAM, save states and hashes alone; the menu stays readable.
Sources: [tuning](../port/tuning.h), machine timing, SDL frontend and menu.

## New Cheats

See [CHEATS.md](CHEATS.md) for **every custom cheat**, every selector choice, supported revisions,
activation timing and limits. Per-game `kFeatureOwn` settings do not become collection-wide switches.
Existing upstream cheats are retained. Tested base revisions do not imply validation of every alternate ROM region.

## Launcher Hardening

The initial window is centered, uses more available desktop space and leaves visible margins instead of
maximizing. Custom controls stay inside their panel. Controller controls and global shortcuts have separate groups.
The fork also retains its portable `user/<id>` format alongside upstream's format, optional tracing,
separate per-game logs, Windows Unicode paths and import of earlier Input Lab saves (distinguishing custom
v6 from official v6; new states use v9). Import keeps board state, not obsolete training metadata.
Old saves next to settings.ini remain discoverable.

The network version distinguishes incompatible custom binaries. Both peers need the same build;
official online cheat restrictions and native clocks remain. The stock updater is disabled specifically
for this fork so it cannot overwrite custom features. This is not proposed as a blanket upstream policy.
The build helper/static core option and regression fixtures support rebuilding the collection.

## Validation and scope

The complete 018 build passed eighteen build/start checks, six regression suites, 10,801 SFA, 746,874 SFA2,
19,441 SFZ2AL and 12,457 Super Turbo assertions; 396 colour cases/32,475 assertions and menu persistence checks.
EQ was compared with a MAME oracle and audio fixtures in 016. The user confirmed features through play.
These are Windows x64 results for the integrated build, not public CI or exhaustive hardware/combat coverage.
Original workflows remain; their recomp-common consistency check detects intentional fork changes.
The MAMEPPK cheat XML is a separate project and is not distributed here.
