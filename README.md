# CPS-2 Recomp — Input Lab

English | [Español](README.es.md)

**Sol1vaN's customized fork of [tmcleroy/cps2recomp](https://github.com/tmcleroy/cps2recomp).**
Version **1.2.0-inputlab.018**, built on upstream **1.2.0**, commit [`f1212bca`](https://github.com/tmcleroy/cps2recomp/commit/f1212bca038f543e30fb106d8f88dabf30374df5).

Two friends can use separate physical keyboards with the same layout, without one keyboard releasing
the other's held keys. Input Lab also adds optional direction neutralization, physical gamepad assignments,
editable global shortcuts, per-game audio and picture controls, and additional Street Fighter cheats.
The original toolkit, eighteen games, renderer, training modes and online system remain credited to
Tommy McLeroy and their contributors. This is an independent fork, not an official release.

**No ROM sets, keys, decrypted programs, translated C++, binaries or personal settings are supplied.**
Use your own matching game sets. The `input-lab` branch contains the customized build; `main` preserves
upstream as copied when the fork was created. Later official changes have not silently been merged into
the user-tested 018 game code: see [the exact comparison](docs/UPSTREAM_DIFFERENCES.md).

## Features

| Feature | What it adds |
|---|---|
| [RawInput Multi Keyboard](docs/INPUT_LAB_CHANGES.md#rawinput-multi-keyboard) | Independent interface state and safe grouping of interfaces belonging to one physical keyboard. Same keys for different players, persistent device assignments. Windows only. |
| [Neutralize Opposing Directions](docs/INPUT_LAB_CHANGES.md#neutralize-opposing-directions) | Optional per-game/player Left+Right and Up+Down cancellation, with MAME-style direction semantics. Includes per-game Custom combinations and autofire. |
| [Physics Assignment Joysticks](docs/INPUT_LAB_CHANGES.md#physics-assignment-joysticks) | Assign a physical controller to a chosen player, including a lone gamepad on P2. Buttons, triggers and signed stick directions can be bound. |
| [Global Bindable Hotkeys](docs/INPUT_LAB_CHANGES.md#global-bindable-hotkeys) | Editable and clearable keyboard/gamepad shortcuts, including Guide/L3/Start+Select and Reset game. Escape remains fixed. |
| [Audio Equalizer](docs/INPUT_LAB_CHANGES.md#audio-equalizer) | Five-band parametric EQ at Game → Sound → Equalizer, saved independently for each game. |
| [Sliders](docs/INPUT_LAB_CHANGES.md#sliders) | Per-game main/audio CPU clocks, video refresh, brightness, contrast and gamma. Inspired by MAMEPPK. |
| [New Cheats](docs/CHEATS.md) | Live damage-profile equalization, Super Turbo boss selection/internal speed/native palettes, and START-based hidden-character selection. |
| [Launcher Hardening](docs/INPUT_LAB_CHANGES.md#launcher-hardening) | Centered initial window with desktop margins, contained Custom controls, portable-data compatibility and safeguards for this fork's updates. |

“Physics Assignment Joysticks” is this fork's requested feature name; it means **physical device assignment**, not changes to game physics.

## Screenshots

See the [feature gallery](docs/SCREENSHOTS.md) for all eight features. These are captures of the actual
Input Lab UI, not mockups. A screenshot explains the controls; it does not by itself prove device isolation,
audio response or gameplay equivalence.

## New Cheats: what each option actually does

The [complete cheat guide](docs/CHEATS.md) explains every custom cheat, its choices, affected games,
activation time and limits. The [Spanish guide](docs/CHEATS.es.md) contains the same explanations.

| Game / validated base set | Custom cheats |
|---|---|
| Street Fighter Alpha — `sfa`, Europe 950727 | Damage hits |
| Street Fighter Alpha 2 — `sfa2u`, USA 960430 | Damage hits; Hidden characters with START |
| Street Fighter Zero 2 Alpha — `sfz2al`, Asia 960826 | Damage hits, including Dramatic Battle's third fighter |
| Super Street Fighter II Turbo — `ssf2xjr1`, Japan 940223 | Damage hits; P1/P2 Select Character, including Shin Akuma; Select Speed; P1/P2 Select Colour |
| Vampire Savior — `vsavj`, Japan 970519 | Hidden characters with START |
| Super Puzzle Fighter II Turbo — `spf2t`, Europe 960529 | Hidden-character cycle with START |

All these cheats are game-specific. New cheat selectors start Off/Vanilla. Equal damage profiles do not
make every character or move deal identical damage. The Shin Akuma bridge preserves native hitboxes,
pushback and juggle rules; it is not a promise that every proposed combo is possible.

## Getting started

Open `cps2recomp-launcher.exe`. Under Controllers, assign different physical keyboards to the players
to reuse the same keys. No input logger is needed. This removes software interference between devices;
each keyboard's own electrical rollover limits still apply. Global shortcuts and devices are shared across
games; input options, cheats, sliders and EQ are saved per game.

See [Windows build instructions](input-lab/BUILD.md), [014–018 release notes](input-lab/README.md),
[full functional differences](docs/INPUT_LAB_CHANGES.md), and [018 validation results](docs/validation/inputlab-018.json).
Do not run `tools/sync-common.sh` on this customized branch: it would replace its intentional shared-UI changes.

The custom changes were validated on **Windows x64**. No equivalent validation is claimed for Linux,
macOS or Android. Both online peers need the same Input Lab version. Online retains the official cheat
restrictions and original clocks. The stock auto-updater is disabled so it cannot replace this fork with
an official binary that lacks its additions.

## Games inherited from upstream

| Game | ROM set | Notes |
| --- | --- | --- |
| [**Super Street Fighter II Turbo**](games/ssf2t/README.md) (1994) | `ssf2xjr1` (Japan 940223) | The tournament revision; Akuma and the old characters without codes |
| [**Street Fighter Alpha**](games/sfa/README.md) (1995) | `sfa` (Europe 950727) | |
| [**Street Fighter Alpha 2**](games/sfa2/README.md) (1996) | `sfa2u` (USA 960430) | Team battle and Marvel-style tag (2 vs 2 for four players) |
| [**Street Fighter Zero 2 Alpha**](games/sfz2al/README.md) (1996) | `sfz2al` (Asia 960826) | Alpha 2's later revision, with the EX characters |
| [**Street Fighter Alpha 3**](games/sfa3/README.md) (1998) | `sfa3u` (USA 980904) | Team battle and tag; Balrog, Juni and Juli without codes |
| [**Super Gem Fighter Mini Mix**](games/pocket_fighter/README.md) (1997) | `sgemf` (USA 970904) | |
| [**X-Men: Children of the Atom**](games/xmcota/README.md) (1994) | `xmcota` (Euro 950331) | Juggernaut, Magneto and Akuma without codes |
| [**Marvel Super Heroes**](games/msh/README.md) (1995) | `msh` (Europe 951024) | Dr. Doom and Thanos without codes |
| [**X-Men vs. Street Fighter**](games/xmvsf/README.md) (1996) | `xmvsf` (Europe 961004) | Tag-team co-op, and 2 vs 2 for four players |
| [**Marvel Super Heroes vs. Street Fighter**](games/mshvsf/README.md) (1997) | `mshvsf` (Europe 970625) | Tag-team co-op, and 2 vs 2 for four players |
| [**Marvel vs. Capcom**](games/mvsc/README.md) (1998) | `mvsc` (Europe 980123) | Tag-team co-op, and 2 vs 2 for four players |
| [**Super Puzzle Fighter II Turbo**](games/puzzle_fighter/README.md) (1996) | `spf2t` (Europe 960529) | A puzzle training mode and CPU battle |
| [**Vampire Savior**](games/vampire_savior/README.md) (1997) | `vsavj` (Japan 970519) | Tag team battle |
| [**Vampire Savior 2**](games/vsav2/README.md) (1997) | `vsav2` (Japan 970913) | Donovan, Phobos and Pyron back |
| [**Vampire Hunter 2**](games/vhunt2/README.md) (1997) | `vhunt2` (Japan 970929) | Night Warriors' cast on Vampire Savior's engine |
| [**Vampire Savior Definitive**](games/vsav_definitive/README.md) (beta) | `vsavj` and `vsav2` | Vampire Savior with all eighteen characters |
| [**Alien vs. Predator**](games/avsp/README.md) (1994) | `avspu` (USA 940520) | Beat-'em-up: three players, a stage to start at, score attack |
| [**Cadillacs and Dinosaurs**](games/dino/README.md) (1993, CPS-1) | `dino` (World 930201) | Beat-'em-up: three players, a stage to start at, score attack |

## Validation and credits

Build 018 compiled and started all eighteen games from its final installation. Six regression suites,
damage/boss/speed checks, 396 palette cases with 32,475 assertions, and menu save/reload checks passed.
With Select Colour Off, 2,100 rendered frames matched build 017. These are local results for the complete
018 build, not a claim that isolated upstream PRs or public CI have already passed the same checks.
The user also confirmed the custom features through play. Hardware and combat coverage are not exhaustive.

[GPL-3.0-only](LICENSE). Original credits remain intact. Audio Equalizer adapts Olivier Galibert's
MAME/MAMEPPK DSP under [BSD-3-Clause](licenses/mame-equalizer.txt); see [third-party notices](THIRD_PARTY_NOTICES.md).
No affiliation with Capcom or the official project is implied.


The [six upstream draft proposals](docs/UPSTREAM_PRS.md) include screenshots, test scope and the shared-library prerequisite.
