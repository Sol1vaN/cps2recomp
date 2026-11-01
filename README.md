# CPS-2 Recomp — Input Lab

**Sol1vaN's customized fork of [tmcleroy/cps2recomp](https://github.com/tmcleroy/cps2recomp).**
Version **1.2.0-inputlab.018**, based on upstream **1.2.0** / [`f1212bca`](https://github.com/tmcleroy/cps2recomp/commit/f1212bca038f543e30fb106d8f88dabf30374df5).

Independent physical keyboards on Windows, opposing-direction neutralization, per-game macros/autofire,
editable keyboard/gamepad shortcuts, a five-band equalizer, timing/picture sliders and additional
Street Fighter cheats. The upstream toolkit, games, training modes, renderer and online systems
remain credited to Tommy McLeroy and their original contributors.

**Source only: no ROM sets, keys, translated game programs, binary packages or personal settings.**
Build with your own matching ROM sets. This fork is not an official upstream release.

## Documentación

- [Todas las diferencias funcionales y cheats](docs/INPUT_LAB_CHANGES.md).
- [Inventario completo de archivos y novedades oficiales posteriores](docs/UPSTREAM_DIFFERENCES.md).
- [Compilación de Input Lab en Windows](input-lab/BUILD.md).
- [Historial y guía de las builds 014–018](input-lab/README.md).
- [Resumen de pruebas locales de la build 018](docs/validation/inputlab-018.json).
- [README oficial de la base 1.2.0](https://github.com/tmcleroy/cps2recomp/blob/f1212bca038f543e30fb106d8f88dabf30374df5/README.md).

La rama **input-lab** contiene la versión personalizada. **main** conserva el upstream copiado al crear
el fork. Se publica exactamente el código de juego validado de la build 018, con documentación y
exclusiones de archivos locales añadidas. No se han mezclado silenciosamente novedades oficiales posteriores.

## Qué añade este fork

| Área | Cambios |
|---|---|
| Multiteclado Windows | RawInput independiente por interfaz y OR dentro del mismo dispositivo físico; los jugadores pueden usar las mismas teclas sin liberaciones cruzadas. |
| Direcciones | Neutralize Opposing Directions (MAME), por juego/jugador: L+R y U+D se cancelan; L+D+R conserva D. |
| Macros y autofire | PPP/KKK, cuatro Custom simultáneas por jugador y autofire por acción con demora 1–30 frames. |
| Mandos | Asignación física por jugador; ambos sticks y sentidos de ejes; un solo mando puede controlar P2. |
| Atajos globales | Teclado/mando editables y borrables; ESC fijo, Reset game, Guide/L3/Start+Select configurables. |
| Audio | Game → Sound → Equalizer: cinco bandas paramétricas por juego, graves/agudos Shelf o Peak, Bypass y Reset All. |
| Sliders | CPU principal/audio, refresco de video, brillo, contraste y gamma, guardados por juego. |
| Cheats | Damage hits en cuatro juegos; Shin Akuma CPU, velocidad interna y paletas por jugador en Super Turbo; selección con START en los tres juegos originales. |
| Launcher/datos | Ventana amplia con márgenes, Custom sin desbordes, modo portátil, importación de estados propios y versión de red diferenciada. |

## Cheats propios

Menú **Game → Cheats**, o página propia del juego en el launcher. Todos son por juego;
los selectores P1/P2 también son independientes entre jugadores.

| Juego / conjunto base validado | Opciones |
|---|---|
| SFA — `sfa`, Europe 950727 | Damage hits |
| SFA2 — `sfa2u`, USA 960430 | Damage hits; formas ocultas con START |
| SFZ2AL — `sfz2al`, Asia 960826 | Damage hits, incluido el tercer luchador de Dramatic Battle |
| Super Turbo — `ssf2xjr1`, Japan 940223 | Damage hits; P1/P2 Select Character con Shin Akuma; Select Speed; P1/P2 Select Colour |
| Vampire Savior — `vsavj`, Japan 970519 | Formas ocultas con START |
| Puzzle Fighter — `spf2t`, Europe 960529 | Ciclo de personajes ocultos con START |

**Damage hits:** Vanilla / Equal to human / Equal to CPU. Usa el perfil actual y su rango adaptativo,
sin forzar el máximo ni cambiar la IA. Conserva modificadores de personaje, movimiento, bloqueo,
vida, combo y azar. PvP normal y CPU vs CPU conservan Vanilla.

**Super Turbo — Select Colour:** los modos LP/MP/HP y SSF2 primary dan negro a Akuma/Shin Akuma;
LK/MK/HK, Start, Hold any P/K y SSF2 alternate dan gris oscuro. Los demás personajes mantienen
sus diez colores. Elegir antes de la próxima ronda; Off deja de forzar la paleta.
El retrato negro de Akuma no muestra el color de la pelea.

**Select Speed** controla el frameskip interno original, separado de overclock/Hz.
**Select Character** se elige antes de confirmar personaje. El puente de Shin Akuma usa recursos
del jefe CPU y comandos humanos; no modifica hitboxes, separación o juggle ni promete equivalencia
1:1 de toda secuencia de combate. [Detalle de cada cheat](docs/INPUT_LAB_CHANGES.md#cheats-por-juego).

## Uso y compatibilidad

La variante se validó en **Windows x64**. RawInput multiteclado requiere Windows;
no se declara validación de estos agregados en macOS, Linux o Android. El código oficial permanece.
Para compilar: [input-lab/BUILD.md](input-lab/BUILD.md). **No ejecutar `tools/sync-common.sh`**
sobre esta rama: reemplazaría sus modificaciones propias de `recomp-common`.

Abrir `cps2recomp-launcher.exe`. En Controllers, asignar un teclado físico distinto a cada jugador
para reutilizar el mismo esquema. Esto no elimina los límites de rollover del hardware.
Los atajos While playing son globales; los perfiles de juego y EQ se guardan por juego.
Los cheats nuevos comienzan apagados y el EQ plano a 0 dB. No hace falta activar un logger.

El actualizador automático oficial está desactivado para no reemplazar el fork con un binario stock.
Para jugar en red ambos extremos necesitan la misma versión Input Lab; se mantienen las reglas
oficiales de cheats y los relojes originales durante la partida online.

## Juegos heredados de la base oficial

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

## Pruebas y alcance

La build 018 compiló y arrancó los **18 juegos** desde su instalación final. Pasaron seis suites
de regresión, las pruebas específicas de daño/jefe/velocidad, 396 casos/32 475 comprobaciones de
paleta y el guardado/recarga del menú. Con Select Colour en Off, 2 100 imágenes coinciden con la 017.
El usuario confirmó el funcionamiento de sus agregados jugando. Son resultados de pruebas locales,
no una afirmación de que el CI público ya haya pasado ni de cobertura de todo hardware/combate.
Los workflows originales se conservan, pero aún no se adaptaron a las diferencias intencionadas
de `recomp-common` ni se validaron para las otras plataformas.

## Licencia y créditos

[GPL-3.0-only](LICENSE). Se conservan los créditos originales de Tommy McLeroy y demás autores.
El EQ adapta el DSP de Olivier Galibert de MAME/MAMEPPK, con [BSD-3-Clause](licenses/mame-equalizer.txt).
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) contiene las atribuciones completas.
No hay afiliación con Capcom ni con el proyecto oficial.
