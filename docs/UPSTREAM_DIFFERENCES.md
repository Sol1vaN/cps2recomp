# Upstream comparison and file inventory

English | [Español](UPSTREAM_DIFFERENCES.es.md)

- User-tested game base: **1.2.0**, [`f1212bca`](https://github.com/tmcleroy/cps2recomp/commit/f1212bca038f543e30fb106d8f88dabf30374df5).
- Contribution baseline observed earlier on 2026-11-01: [`ba4f4e50`](https://github.com/tmcleroy/cps2recomp/commit/ba4f4e504e589fdc40653323581a1b3bf2c4ebf0), eleven commits later.
- Those later changes are **not merged into Input Lab 018**. `input-lab` is the custom branch; `main` preserves the upstream copy made when forking.

The [functional comparison](INPUT_LAB_CHANGES.md) uses the reproducible tested base, not a moving claim
that the fork includes every future upstream improvement. [Compare the two official references](https://github.com/tmcleroy/cps2recomp/compare/f1212bca038f543e30fb106d8f88dabf30374df5...ba4f4e504e589fdc40653323581a1b3bf2c4ebf0).

Upstream continued moving while the PRs were prepared: [`0d891325`](https://github.com/tmcleroy/cps2recomp/commit/0d8913252062029265b8c8c588ca3d90e041df5b) adds The Punisher, following Darkstalkers. The inventory below is deliberately pinned to `ba4f4e50`, not presented as a complete comparison against an ever-moving main. The six focused contributions start at that pinned baseline and target official main; they do not upgrade the user-tested 018 build.

## Later official changes awaiting integration

| Official improvement | Input Lab 018 status |
|---|---|
| Optional Display VSync, retaining board pacing | Not incorporated. |
| F12 captures the presented picture | New official shortcut/path not incorporated; earlier headless capture tools remain. |
| gamecontrollerdb.txt for SDL-unrecognized pads | New loader not incorporated; physical assignment/signed axes are independent fork features. |
| Windows X-Men COTA folder correction | Not incorporated; custom portable user/xmcota passed local start checks. |
| Windows system proxy for online/updates | Not incorporated. |
| Android update notification/download | Not incorporated; the fork deliberately disables the stock updater. |
| Excluding generated example files | Not incorporated. |

Replacing the whole frontend or recomp-common would overwrite custom input, shortcuts, profiles and
persistence. Future integration needs a reviewed merge and fresh validation.

### Later upstream commits

- [`ba4f4e50`](https://github.com/tmcleroy/cps2recomp/commit/ba4f4e504e589fdc40653323581a1b3bf2c4ebf0) Merge pull request #218 from tmcleroy/vsync-option
- [`8165d75e`](https://github.com/tmcleroy/cps2recomp/commit/8165d75e68958d51205866c1252bedb0dab131f6) Display's VSync: each picture at the screen's refresh, never torn (#185)
- [`ff52ebf7`](https://github.com/tmcleroy/cps2recomp/commit/ff52ebf7e9f5c62d7815f48dde8c146629ad7237) Merge pull request #217 from tmcleroy/screenshot-mappings
- [`33eb0c79`](https://github.com/tmcleroy/cps2recomp/commit/33eb0c79008e00f309de7a615f7b0c22a39fa8fb) F12 saves a screenshot; gamecontrollerdb.txt for controllers SDL doesn't know (#31, #213)
- [`892db6ce`](https://github.com/tmcleroy/cps2recomp/commit/892db6cef2851a1b3efd735aca04c48a836cbf49) Merge pull request #216 from tmcleroy/xmcota-windows-folder
- [`9e6f2a40`](https://github.com/tmcleroy/cps2recomp/commit/9e6f2a4075a3023a9d05a0acfc66792fb25a60eb) recomp-common synced to main (#49 merged)
- [`04358aad`](https://github.com/tmcleroy/cps2recomp/commit/04358aadeff1b14bd090287bd7ac42b1cccf5bab) Not the example's generated files (make check's)
- [`6fa28fc3`](https://github.com/tmcleroy/cps2recomp/commit/6fa28fc3e80f95ea60125899268ab8016d74bb99) Windows: X-Men COTA's folder, and the system's proxy for the online service (#212, #194)
- [`427b38f6`](https://github.com/tmcleroy/cps2recomp/commit/427b38f68ec93b58c9f8444c533240d02d2e8411) Merge pull request #210 from tmcleroy/android-update-notice
- [`46725ce5`](https://github.com/tmcleroy/cps2recomp/commit/46725ce5a8eb18784870b73d9011bf02c9e73434) recomp-common synced to main (#45 merged)
- [`309ee318`](https://github.com/tmcleroy/cps2recomp/commit/309ee31854de0f1d769a2afe9abb9fe3e30e343d) Android says when a new version is out (#209)

## Upstream files changed after the tested base

- `M` `CHANGELOG.md`
- `M` `docs/FEATURES.md`
- `M` `launcher/CMakeLists.txt`
- `M` `port/frontend_sdl.cpp`
- `M` `port/main.cpp`
- `M` `port/png_write.h`
- `M` `port/port.h`
- `M` `port/present.cpp`
- `M` `port/present.h`
- `M` `port/shading.cpp`
- `M` `port/shading.h`
- `M` `recomp-common/COMMIT`
- `M` `recomp-common/folders.h`
- `A` `recomp-common/gamepads.h`
- `M` `recomp-common/launcher/src/app.cpp`
- `M` `recomp-common/launcher/src/app.h`
- `M` `recomp-common/launcher/src/launcher.h`
- `M` `recomp-common/launcher/src/main.cpp`
- `M` `recomp-common/launcher/src/update.cpp`
- `M` `recomp-common/launcher/src/update.h`
- `A` `recomp-common/proxy.h`

## Complete custom file inventory against the tested base

Every added/modified file is listed. Behavioral details are in [INPUT_LAB_CHANGES.md](INPUT_LAB_CHANGES.md).

| Status | File | Area |
|---|---|---|
| Modified | [.gitignore](../.gitignore) | Integration / build / compatibility |
| Modified | [CMakeLists.txt](../CMakeLists.txt) | Integration / build / compatibility |
| Modified | [README.md](../README.md) | Documentation / evidence |
| Modified | [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) | Integration / build / compatibility |
| Modified | [VERSION](../VERSION) | Integration / build / compatibility |
| Added | [docs/INPUT_LAB_CHANGES.md](../docs/INPUT_LAB_CHANGES.md) | Documentation / evidence |
| Added | [docs/UPSTREAM_DIFFERENCES.md](../docs/UPSTREAM_DIFFERENCES.md) | Documentation / evidence |
| Added | [docs/validation/inputlab-018.json](../docs/validation/inputlab-018.json) | Documentation / evidence |
| Added | [games/puzzle_fighter/src/hidden_selection.h](../games/puzzle_fighter/src/hidden_selection.h) | Cheats / character selection |
| Modified | [games/puzzle_fighter/src/select.cpp](../games/puzzle_fighter/src/select.cpp) | Cheats / character selection |
| Modified | [games/sfa/src/game.cpp](../games/sfa/src/game.cpp) | Cheats / character selection |
| Added | [games/sfa/src/inputlab_damage.cpp](../games/sfa/src/inputlab_damage.cpp) | Cheats / character selection |
| Modified | [games/sfa2/src/damage.cpp](../games/sfa2/src/damage.cpp) | Cheats / character selection |
| Modified | [games/sfa2/src/game.cpp](../games/sfa2/src/game.cpp) | Cheats / character selection |
| Added | [games/sfa2/src/hidden_selection.h](../games/sfa2/src/hidden_selection.h) | Cheats / character selection |
| Modified | [games/sfa2/src/input.cpp](../games/sfa2/src/input.cpp) | Cheats / character selection |
| Modified | [games/sfa2/src/training.cpp](../games/sfa2/src/training.cpp) | Cheats / character selection |
| Added | [games/sfa2/tests/damage_tests.cpp](../games/sfa2/tests/damage_tests.cpp) | Tests |
| Added | [games/sfz2al/src/inputlab_damage.cpp](../games/sfz2al/src/inputlab_damage.cpp) | Cheats / character selection |
| Modified | [games/ssf2t/src/choosing.cpp](../games/ssf2t/src/choosing.cpp) | Cheats / character selection |
| Modified | [games/ssf2t/src/game.cpp](../games/ssf2t/src/game.cpp) | Cheats / character selection |
| Added | [games/ssf2t/src/inputlab_cheats.cpp](../games/ssf2t/src/inputlab_cheats.cpp) | Cheats / character selection |
| Modified | [games/vampire_savior/src/choosing.cpp](../games/vampire_savior/src/choosing.cpp) | Cheats / character selection |
| Added | [games/vampire_savior/src/hidden_selection.h](../games/vampire_savior/src/hidden_selection.h) | Cheats / character selection |
| Added | [input-lab/BUILD.md](../input-lab/BUILD.md) | Documentation / evidence |
| Added | [input-lab/README.md](../input-lab/README.md) | Documentation / evidence |
| Added | [input-lab/core/CMakeLists.txt](../input-lab/core/CMakeLists.txt) | Integration / build / compatibility |
| Added | [input-lab/entries/avsp.txt](../input-lab/entries/avsp.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/avsp.z80.txt](../input-lab/entries/avsp.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/dino.txt](../input-lab/entries/dino.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/dino.z80.txt](../input-lab/entries/dino.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/msh.txt](../input-lab/entries/msh.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/msh.z80.txt](../input-lab/entries/msh.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/mshvsf.txt](../input-lab/entries/mshvsf.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/mshvsf.z80.txt](../input-lab/entries/mshvsf.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/mvsc.txt](../input-lab/entries/mvsc.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/mvsc.z80.txt](../input-lab/entries/mvsc.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/pocket_fighter.txt](../input-lab/entries/pocket_fighter.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/pocket_fighter.z80.txt](../input-lab/entries/pocket_fighter.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/puzzle_fighter.txt](../input-lab/entries/puzzle_fighter.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/puzzle_fighter.z80.txt](../input-lab/entries/puzzle_fighter.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/sfa.txt](../input-lab/entries/sfa.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/sfa.z80.txt](../input-lab/entries/sfa.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/sfa2.txt](../input-lab/entries/sfa2.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/sfa2.z80.txt](../input-lab/entries/sfa2.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/sfa3.txt](../input-lab/entries/sfa3.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/sfa3.z80.txt](../input-lab/entries/sfa3.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/sfz2al.txt](../input-lab/entries/sfz2al.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/sfz2al.z80.txt](../input-lab/entries/sfz2al.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/ssf2t.txt](../input-lab/entries/ssf2t.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/ssf2t.z80.txt](../input-lab/entries/ssf2t.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/vampire_savior.txt](../input-lab/entries/vampire_savior.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/vampire_savior.z80.txt](../input-lab/entries/vampire_savior.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/vhunt2.txt](../input-lab/entries/vhunt2.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/vhunt2.z80.txt](../input-lab/entries/vhunt2.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/vsav2.txt](../input-lab/entries/vsav2.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/vsav2.z80.txt](../input-lab/entries/vsav2.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/vsav_definitive.txt](../input-lab/entries/vsav_definitive.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/vsav_definitive.z80.txt](../input-lab/entries/vsav_definitive.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/xmcota.txt](../input-lab/entries/xmcota.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/xmcota.z80.txt](../input-lab/entries/xmcota.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/xmvsf.txt](../input-lab/entries/xmvsf.txt) | Translation inputs; addresses only |
| Added | [input-lab/entries/xmvsf.z80.txt](../input-lab/entries/xmvsf.z80.txt) | Translation inputs; addresses only |
| Added | [input-lab/rebuild.py](../input-lab/rebuild.py) | Integration / build / compatibility |
| Added | [input-lab/tests/CMakeLists.txt](../input-lab/tests/CMakeLists.txt) | Tests |
| Added | [input-lab/tests/collection_input_tests.cpp](../input-lab/tests/collection_input_tests.cpp) | Tests |
| Added | [input-lab/tests/damage_port_tests.cpp](../input-lab/tests/damage_port_tests.cpp) | Tests |
| Added | [input-lab/tests/equalizer_tests.cpp](../input-lab/tests/equalizer_tests.cpp) | Tests |
| Added | [input-lab/tests/mame_equalizer_oracle.h](../input-lab/tests/mame_equalizer_oracle.h) | Tests |
| Added | [input-lab/tests/pad_hotkey_tests.cpp](../input-lab/tests/pad_hotkey_tests.cpp) | Tests |
| Added | [input-lab/tests/ssf2t_colour_tests.cpp](../input-lab/tests/ssf2t_colour_tests.cpp) | Tests |
| Modified | [launcher/CMakeLists.txt](../launcher/CMakeLists.txt) | Launcher / persistence / UI |
| Modified | [launcher/src/description.cpp](../launcher/src/description.cpp) | Launcher / persistence / UI |
| Added | [launcher/src/global_controls.cpp](../launcher/src/global_controls.cpp) | Global shortcuts |
| Added | [launcher/src/global_controls.h](../launcher/src/global_controls.h) | Global shortcuts |
| Added | [launcher/src/input_controls.cpp](../launcher/src/input_controls.cpp) | Profiles / macros / gamepads |
| Added | [launcher/src/input_controls.h](../launcher/src/input_controls.h) | Profiles / macros / gamepads |
| Added | [launcher/src/keyboard_controls.cpp](../launcher/src/keyboard_controls.cpp) | Keyboards / physical groups |
| Added | [licenses/mame-equalizer.txt](../licenses/mame-equalizer.txt) | EQ / audio / attribution |
| Modified | [machine/machine.cpp](../machine/machine.cpp) | Clocks / sliders / states |
| Modified | [machine/machine.h](../machine/machine.h) | Clocks / sliders / states |
| Modified | [machine/romset.cpp](../machine/romset.cpp) | Integration / build / compatibility |
| Modified | [port/bindings.h](../port/bindings.h) | Profiles / macros / gamepads |
| Added | [port/equalizer.cpp](../port/equalizer.cpp) | EQ / audio / attribution |
| Added | [port/equalizer.h](../port/equalizer.h) | EQ / audio / attribution |
| Modified | [port/frontend_sdl.cpp](../port/frontend_sdl.cpp) | Integration / build / compatibility |
| Added | [port/gamepad_assignment.h](../port/gamepad_assignment.h) | Integration / build / compatibility |
| Added | [port/gamepad_input.h](../port/gamepad_input.h) | Integration / build / compatibility |
| Added | [port/gamepad_tests.cpp](../port/gamepad_tests.cpp) | Tests |
| Added | [port/hotkeys.cpp](../port/hotkeys.cpp) | Global shortcuts |
| Added | [port/hotkeys.h](../port/hotkeys.h) | Global shortcuts |
| Added | [port/input_lab.manifest](../port/input_lab.manifest) | Integration / build / compatibility |
| Added | [port/input_lab.rc](../port/input_lab.rc) | Integration / build / compatibility |
| Added | [port/input_profile.cpp](../port/input_profile.cpp) | Profiles / macros / gamepads |
| Added | [port/input_profile.h](../port/input_profile.h) | Profiles / macros / gamepads |
| Added | [port/input_profile_tests.cpp](../port/input_profile_tests.cpp) | Tests |
| Added | [port/input_trace.cpp](../port/input_trace.cpp) | Integration / build / compatibility |
| Added | [port/input_trace.h](../port/input_trace.h) | Integration / build / compatibility |
| Added | [port/keyboard_tests.cpp](../port/keyboard_tests.cpp) | Tests |
| Added | [port/keyboards.cpp](../port/keyboards.cpp) | Keyboards / physical groups |
| Added | [port/keyboards.h](../port/keyboards.h) | Keyboards / physical groups |
| Modified | [port/main.cpp](../port/main.cpp) | Integration / build / compatibility |
| Modified | [port/menu.cpp](../port/menu.cpp) | Menu / sliders / EQ |
| Modified | [port/menu.h](../port/menu.h) | Integration / build / compatibility |
| Added | [port/pad_hotkeys.h](../port/pad_hotkeys.h) | Global shortcuts |
| Modified | [port/port.h](../port/port.h) | Integration / build / compatibility |
| Added | [port/raw_keyboards.cpp](../port/raw_keyboards.cpp) | Keyboards / physical groups |
| Added | [port/raw_keyboards.h](../port/raw_keyboards.h) | Keyboards / physical groups |
| Added | [port/tuning.cpp](../port/tuning.cpp) | Clocks / sliders / states |
| Added | [port/tuning.h](../port/tuning.h) | Clocks / sliders / states |
| Added | [port/window_layout.h](../port/window_layout.h) | Launcher / persistence / UI |
| Modified | [recomp-common/launcher/launcher.cmake](../recomp-common/launcher/launcher.cmake) | Launcher / persistence / UI |
| Modified | [recomp-common/launcher/src/app.cpp](../recomp-common/launcher/src/app.cpp) | Launcher / persistence / UI |
| Modified | [recomp-common/launcher/src/app.h](../recomp-common/launcher/src/app.h) | Launcher / persistence / UI |
| Modified | [recomp-common/launcher/src/bindings.h](../recomp-common/launcher/src/bindings.h) | Profiles / macros / gamepads |
| Modified | [recomp-common/launcher/src/launcher.h](../recomp-common/launcher/src/launcher.h) | Launcher / persistence / UI |
| Modified | [recomp-common/launcher/src/main.cpp](../recomp-common/launcher/src/main.cpp) | Launcher / persistence / UI |
| Modified | [recomp-common/launcher/src/ui.cpp](../recomp-common/launcher/src/ui.cpp) | Launcher / persistence / UI |
| Modified | [recomp-common/launcher/src/ui.h](../recomp-common/launcher/src/ui.h) | Launcher / persistence / UI |
| Modified | [recomp-common/menu.cpp](../recomp-common/menu.cpp) | Menu / sliders / EQ |
| Modified | [recomp-common/menu.h](../recomp-common/menu.h) | Menu / sliders / EQ |
| Added | [runtime/damage_policy.h](../runtime/damage_policy.h) | Cheats / character selection |
| Modified | [sound/driver.h](../sound/driver.h) | EQ / audio / attribution |

## Published contents and exclusions

The game source is the user-tested 018 source archive plus publication documentation, a sanitized
validation summary and local-file exclusions. Translation tools and port sources are included.
Historical input-lab/entries build inputs contain addresses rather than translated instructions; they
are excluded from upstream contributions under upstream's policy. GPL-3.0 and EQ BSD-3-Clause notices remain.

Local work/out/migration/package folders, ROMs, keys, decrypted/translated programs, local binaries,
personal profiles, input traces, device identifiers, saves and backups are excluded. External MAMEPPK,
PCSX2 and DuckStation changes are separate projects. The native custom cheats are implemented in this fork.
