# Input Lab 014–018 release guide

English | [Español: complete historical notes](README.es.md)

Upstream base: CPS2Recomp 1.2.0, `f1212bca038f543e30fb106d8f88dabf30374df5`.
Current custom version: **1.2.0-inputlab.018**.

| Build | Main additions |
|---|---|
| 014 | Eighteen-game collection integration; independent keyboard interface groups, physical pad assignment, per-game direction neutralization/Custom/autofire, global keyboard shortcuts, sliders and save compatibility. |
| 015 | Editable/clearable gamepad shortcuts, including all three menu accesses; context-aware conflicts and persistent empty bindings. |
| 016 | Per-game five-band Audio Equalizer at Game → Sound → Equalizer, for board sound and custom music. |
| 017 | Damage hits for SFA/SFA2/SFZ2AL/Super Turbo, third-fighter Dramatic Battle support, Super Turbo character/boss selection and internal speed. |
| 018 | P1/P2 Select Colour for Super Turbo, correctly mapping Akuma/Shin Akuma's two native palettes to the ten ordinary selector choices. |

See [all eight named features](../docs/INPUT_LAB_CHANGES.md), [every cheat explained](../docs/CHEATS.md),
[screenshots](../docs/SCREENSHOTS.md), [build instructions](BUILD.md), and [validation summary](../docs/validation/inputlab-018.json).
The Spanish alternative preserves the detailed historical build-by-build notes.

Open Jugar.cmd or cps2recomp-launcher.exe for normal play, without a keyboard logger. Settings live in
`user/<id>`; devices and shortcuts are global. Older packaged builds were retained with backups during migration.
Both online peers need the same custom version. Online uses original clocks and upstream cheat restrictions;
offline tuning is preserved. The stock updater is disabled to avoid replacing custom additions.

The source includes intentional shared-UI changes. Do not overwrite them with tools/sync-common.sh.
Generated game programs, ROMs, keys, local builds, private device identifiers and personal saves/settings
are excluded. Translation inputs under entries/ are historical local build inputs and are **not intended
for upstream PRs**; upstream's contribution policy explicitly excludes entry lists.
