# Building Input Lab 018 on Windows

English | [Español](BUILD.es.md)

Base: v1.2.0, `f1212bca038f543e30fb106d8f88dabf30374df5`. The fork includes its intentional
recomp-common modifications. **Do not run tools/sync-common.sh over this branch.**

## Dependencies

- Python 3, CMake 3.28+ and Ninja.
- LLVM/Clang 22.1.8 for x86_64-w64-windows-gnu, MinGW headers/libraries and windres.
- SDL 3.4.16 SDK with SDL3Config.cmake/import library, and zlib 1.3.1.
- Dear ImGui v1.92.9b and Inter 4.1; CMake downloads the pinned official revisions unless local checkouts are specified.
- SDL3.dll and zlib1.dll on PATH when running the initial decryption executable.
- Your own matching sets as described by games/*/game.cfg. No ROMs, keys or decrypted programs are supplied.

From the source root, replace the dependency paths:

```powershell
$env:PYTHONUTF8 = '1'
python -X utf8 input-lab/rebuild.py --roms 'D:/ROMs' --sdl 'D:/SDL3/cmake' --zlib-include 'D:/zlib/include' --zlib-library 'D:/zlib/lib/zlib.lib' --cc 'D:/llvm/bin/clang.exe' --cxx 'D:/llvm/bin/clang++.exe' --rc 'D:/llvm/bin/windres.exe' --jobs 4
```

Optional: --cmake, --imgui, --inter, --games sfa2 puzzle_fighter. Output: out/rebuild/<id> and
out/rebuild/launcher. The first untranslated executable dumps the user's ROM; the translators use local
entries/ inputs to generate native C++, then rebuild. Definitive also needs the Vampire Savior 2 set.
Do not share work/ between different games/revisions. The helper reproduces the commands used for this
build; a second independent complete clean rebuild through the helper has not been claimed.

## Tests

Configure input-lab/tests with the same dependencies and
`-DINPUTLAB_CORE_LIBRARY=<path>/out/rebuild/core/libcps2_inputlab_core.a`.
Build/run keyboard_tests --native, gamepad_tests, input_profile_tests, collection_input_tests,
pad_hotkey_tests and equalizer_tests. DLLs must be available when running them.
Game policy tests take work/<game>/program.bin.data and work/<game>/program.bin:
sfa_policy_tests, sfa2_damage_tests, sfz2al_policy_tests, ssf2t_policy_tests and ssf2t_colour_tests.
The colour test runs the original palette loader and checks copied VRAM words.

EQ tests compare the DSP with the attributed MAMEPPK oracle, including response, channel independence,
block continuity and SDL conversion. Audio fixtures exercise flat, bypass, active board audio and 48 kHz
custom music. Game hashes remain unchanged by EQ. Automated results do not replace real device playtests.
See the [018 validation record](../docs/validation/inputlab-018.json).

## Packaging

Place the eighteen executables and launcher beside the required DLLs, licenses and shaders from the
verified official v1.2.0 release. Retain input-lab-portable.txt to use user/<id>. Do not distribute work/,
dumps or translated game C++. Retain all original licenses, GPL-3.0 source availability, and the EQ
BSD-3-Clause attribution in licenses/mame-equalizer.txt and THIRD_PARTY_NOTICES.md.
