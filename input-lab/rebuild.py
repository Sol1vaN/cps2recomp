#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Rebuild the Windows collection from source and the user's own ROMs."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--roms', required=True, type=Path, help='Folder containing the canonical ROM ZIPs')
    p.add_argument('--sdl', required=True, type=Path, help='SDL3 CMake package directory')
    p.add_argument('--zlib-include', required=True, type=Path)
    p.add_argument('--zlib-library', required=True, type=Path)
    p.add_argument('--cmake', default='cmake')
    p.add_argument('--cc', default='clang')
    p.add_argument('--cxx', default='clang++')
    p.add_argument('--rc', default='windres')
    p.add_argument('--jobs', type=int, default=4)
    p.add_argument('--imgui', type=Path, help='Optional local Dear ImGui v1.92.9b checkout')
    p.add_argument('--inter', type=Path, help='Optional local Inter 4.1 checkout')
    p.add_argument('--games', nargs='+', help='Optional subset; default all 18')
    a = p.parse_args()
    root = Path(__file__).resolve().parents[1]
    out = root / 'out/rebuild'
    env = os.environ.copy()
    env['PYTHONUTF8'] = '1'
    env.pop('CPS2_KEYBOARD_TRACE_DIR', None)
    env['CPS2RECOMP_HOME'] = str(out / 'scratch-user')
    games = a.games or sorted(x.parent.name for x in (root / 'games').glob('*/game.cfg'))
    common = ['-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
              f'-DCMAKE_C_COMPILER={a.cc}', f'-DCMAKE_CXX_COMPILER={a.cxx}',
              f'-DCMAKE_RC_COMPILER={a.rc}', '-DCMAKE_EXE_LINKER_FLAGS=-static-libstdc++ -static-libgcc',
              f'-DSDL3_DIR={a.sdl.resolve()}', f'-DZLIB_INCLUDE_DIR={a.zlib_include.resolve()}',
              f'-DZLIB_LIBRARY={a.zlib_library.resolve()}']

    def run(*args):
        subprocess.run([str(x) for x in args], cwd=root, env=env, check=True)

    def configure(src, dest, *args):
        run(a.cmake, '-S', src, '-B', dest, *common, *args)

    def build(dest, target=None):
        run(a.cmake, '--build', dest, '--parallel', a.jobs,
            *(['--target', target] if target else []))

    core_dir = out / 'core'
    configure(root / 'input-lab/core', core_dir)
    build(core_dir)
    core = core_dir / 'libcps2_inputlab_core.a'
    for game in games:
        cfg = {}
        for line in (root / 'games' / game / 'game.cfg').read_text(encoding='utf8').splitlines():
            if '=' in line and not line.lstrip().startswith('#'):
                k, v = line.split('=', 1)
                cfg[k.strip()] = v.strip()
        work = root / 'work' / game
        gen = work / 'gen'
        gen.mkdir(parents=True, exist_ok=True)
        run(sys.executable, 'tools/gamecfg.py', '--header', gen / 'game_info.h', game)
        dest = out / game
        options = [f'-DGAME={game}', f'-DGAME_DIR={root}/games/{game}',
                   f'-DWORK_DIR={work}', f'-DINPUTLAB_CORE_LIBRARY={core}']
        if not (work / 'program.bin.z80').exists():
            # First build is the source interpreter: it decrypts/dumps the user's set.
            configure(root, dest, *options)
            build(dest, game)
            exe = dest / (game + '.exe')
            for dll in [a.sdl.parent / 'lib/x64/SDL3.dll', a.zlib_library.parent / 'zlib1.dll']:
                if dll.exists():
                    shutil.copy2(dll, dest / dll.name)
            # SDL3.dll and zlib1.dll may instead be supplied through PATH.
            run(exe, '--rom', a.roms.resolve(), '--dump-program', work / 'program.bin',
                *(['--rom2', a.roms.resolve()] if cfg.get('extra_set') else []))
        if cfg.get('extra_set') and (root / 'games' / game / 'graft.cfg').exists():
            extra = work / 'extra'
            extra.mkdir(exist_ok=True)
            for suffix in ['', '.data']:
                shutil.copy2(work / ('program.bin.extra' + suffix), extra / ('program.bin' + suffix))
            run(sys.executable, 'tools/portmap.py', '--from', work, '--to', extra,
                '--out', extra / 'portmap.txt', '--entries', extra / 'entries.txt')
            run(sys.executable, 'tools/graft.py', '--spec', root / 'games' / game / 'graft.cfg',
                '--work', work, '--base', cfg['extra_base'], '--out', gen / 'graft_data.h')
        (gen / 'region_hooks.txt').write_text(
            f"{int(cfg['region_at'], 16) + 6:X}\n" if cfg.get('region_at') else '', encoding='utf8')
        sources = os.pathsep.join(str(root / 'games' / g / 'src')
                                  for g in [game] + cfg.get('sources', '').split())
        run(sys.executable, 'tools/recomp.py', '--program', work / 'program.bin',
            '--entries', root / 'input-lab/entries' / (game + '.txt'), '--out', gen,
            '--limit', cfg.get('code_end', '400000'), '--data', work / 'program.bin.data',
            '--hook-sources', sources, '--places', root / 'games' / game / 'family.h',
            '--hooks', os.pathsep.join(str(gen / f) for f in ['graft_data_hooks.txt', 'region_hooks.txt']))
        run(sys.executable, 'tools/recompz80.py', '--program', work / 'program.bin.z80',
            '--entries', root / 'input-lab/entries' / (game + '.z80.txt'), '--out', gen,
            *(['--opcodes', work / 'program.bin.z80ops'] if (work / 'program.bin.z80ops').exists() else []))
        configure(root, dest, *options)
        build(dest, game)
    launcher = out / 'launcher'
    options = ['-DCOLLECTION=ON', '-DGAME_NAME=CPS-2 Recomp', '-DGAME_EXE=cps2recomp',
               '-DAPP_ID=io.github.tmcleroy.cps2recomp', '-DHOME_ENV=CPS2RECOMP_HOME',
               '-DDISCLAIMER=Not affiliated with Capcom.', '-DGAMES=' + ';'.join(games),
               f'-DPython3_EXECUTABLE={sys.executable}']
    for name, folder in [('IMGUI', a.imgui), ('INTER', a.inter)]:
        if folder:
            options.append(f'-DFETCHCONTENT_SOURCE_DIR_{name}={folder.resolve()}')
    configure(root / 'launcher', launcher, *options)
    build(launcher)


if __name__ == '__main__':
    main()
