# SPDX-License-Identifier: GPL-3.0-only
# Compile the unchanged, game-independent runtime once for collection builds.
cmake_minimum_required(VERSION 3.28)
project(cps2_inputlab_core LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SDL3 REQUIRED CONFIG)
find_package(ZLIB REQUIRED)
get_filename_component(CPS2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
add_library(cps2_inputlab_core STATIC
    "${CPS2_ROOT}/video/video.cpp"
    "${CPS2_ROOT}/sound/z80.cpp"
    "${CPS2_ROOT}/sound/qsound.cpp"
    "${CPS2_ROOT}/sound/driver.cpp"
    "${CPS2_ROOT}/machine/m68k.cpp"
    "${CPS2_ROOT}/machine/machine.cpp"
    "${CPS2_ROOT}/machine/sound_board.cpp"
    "${CPS2_ROOT}/machine/soundtrack.cpp"
    "${CPS2_ROOT}/machine/eeprom.cpp"
    "${CPS2_ROOT}/machine/romset.cpp"
    "${CPS2_ROOT}/machine/cps2crypt.cpp"
    "${CPS2_ROOT}/machine/kabuki.cpp"
    "${CPS2_ROOT}/runtime/recomp.cpp"
    "${CPS2_ROOT}/runtime/recomp_z80.cpp"
    "${CPS2_ROOT}/port/input_trace.cpp"
    "${CPS2_ROOT}/port/keyboards.cpp"
    "${CPS2_ROOT}/port/raw_keyboards.cpp"
    "${CPS2_ROOT}/port/input_profile.cpp"
    "${CPS2_ROOT}/port/hotkeys.cpp"
    "${CPS2_ROOT}/port/tuning.cpp"
    "${CPS2_ROOT}/port/equalizer.cpp"
    "${CPS2_ROOT}/port/present.cpp"
    "${CPS2_ROOT}/port/shading.cpp"
    "${CPS2_ROOT}/port/zoom.cpp"
    "${CPS2_ROOT}/port/menu.cpp"
    "${CPS2_ROOT}/port/hud.cpp"
    "${CPS2_ROOT}/port/soundtrack.cpp"
    "${CPS2_ROOT}/recomp-common/launcher/src/online.cpp"
    "${CPS2_ROOT}/recomp-common/launcher/src/run_program.cpp"
    "${CPS2_ROOT}/port/decoders.cpp"
    "${CPS2_ROOT}/recomp-common/menu.cpp"
    "${CPS2_ROOT}/recomp-common/settings.cpp"
    "${CPS2_ROOT}/runtime/game.cpp"
    "${CPS2_ROOT}/runtime/board.cpp"
    "${CPS2_ROOT}/runtime/spare.cpp"
    "${CPS2_ROOT}/fight/frames.cpp"
    "${CPS2_ROOT}/fight/training.cpp"
    "${CPS2_ROOT}/fight/colours.cpp"
    "${CPS2_ROOT}/fight/colour_editor.cpp"
    "${CPS2_ROOT}/fight/battle.cpp"
    "${CPS2_ROOT}/fight/alternative.cpp"
    "${CPS2_ROOT}/fight/session.cpp"
    "${CPS2_ROOT}/fight/tag.cpp"
    "${CPS2_ROOT}/fight/marvel.cpp"
    "${CPS2_ROOT}/beat/modes.cpp"
    "${CPS2_ROOT}/beat/model.cpp"
    "${CPS2_ROOT}/model/model.cpp"
)
target_include_directories(cps2_inputlab_core PRIVATE "${CPS2_ROOT}/runtime" "${CPS2_ROOT}/port" "${CPS2_ROOT}/machine" "${CPS2_ROOT}/fight" "${CPS2_ROOT}/beat" "${CPS2_ROOT}/model")
target_link_libraries(cps2_inputlab_core PRIVATE SDL3::SDL3 ZLIB::ZLIB)
target_compile_options(cps2_inputlab_core PRIVATE -fsigned-char -Wall -Wextra -Wno-unused-parameter -Wno-unused-label -Wno-unused-variable -Wno-parentheses-equality)
