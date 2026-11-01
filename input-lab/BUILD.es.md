[English](BUILD.md) | Español

# Reconstruir Input Lab 018

Base: v1.2.0, commit `f1212bca038f543e30fb106d8f88dabf30374df5`.
El ZIP contiene las fuentes completas, incluidas las modificaciones de `recomp-common`.
No ejecutar `tools/sync-common.sh`: reemplazaría las modificaciones propias.
El parche adjunto se aplica con `git apply` sobre el commit indicado.

## Dependencias de la compilación de Windows

- Python 3, CMake 3.28 o posterior y Ninja.
- LLVM/Clang 22.1.8, destino x86_64-w64-windows-gnu, con sus cabeceras/librerías MinGW y windres.
- SDL 3.4.16 (SDK con SDL3Config.cmake e import library), zlib 1.3.1.
- Dear ImGui v1.92.9b e Inter 4.1; CMake descarga las revisiones oficiales si no se indican checkouts locales.
- SDL3.dll y zlib1.dll en PATH para ejecutar el primer binario de descifrado.
- Los conjuntos propios correspondientes a `games/*/game.cfg`. No se incluyen ROMs ni programas descifrados en las fuentes.

Ejemplo, desde la raíz de las fuentes, sustituyendo las rutas por las de las dependencias:

```powershell
$env:PYTHONUTF8 = '1'
python -X utf8 input-lab/rebuild.py --roms 'D:/ROMs' --sdl 'D:/SDL3/cmake' --zlib-include 'D:/zlib/include' --zlib-library 'D:/zlib/lib/zlib.lib' --cc 'D:/llvm/bin/clang.exe' --cxx 'D:/llvm/bin/clang++.exe' --rc 'D:/llvm/bin/windres.exe' --jobs 4
```

Opciones: `--cmake`, `--imgui`, `--inter`, `--games sfa2 puzzle_fighter`.
El helper utiliza los mismos comandos de generación/CMake empleados para esta build;
la reconstrucción completa desde cero mediante este helper no se ha repetido como una segunda compilación independiente.
Los binarios salen en `out/rebuild/<id>` y `out/rebuild/launcher`.
El primer ejecutable sin traducciones vuelca la ROM; los traductores generan el C++
con las entradas de `input-lab/entries` y se recompila como código nativo.
Vampire Savior Definitive necesita también el conjunto de Vampire Savior 2.
No reutilizar `work` entre ROMs/revisiones distintas; usar otra copia de las fuentes.

## Pruebas

Configurar `input-lab/tests` con las mismas dependencias y
`-DINPUTLAB_CORE_LIBRARY=<ruta>/out/rebuild/core/libcps2_inputlab_core.a`.
Construir y ejecutar `keyboard_tests --native`, `gamepad_tests`, `input_profile_tests`,
`collection_input_tests`, `pad_hotkey_tests`, `equalizer_tests`; para daño, `sfa2_damage_tests work/sfa2/program.bin.data work/sfa2/program.bin`.
Las DLL también deben estar disponibles al ejecutar las pruebas.
Para los nuevos ports: `sfa_policy_tests`, `sfz2al_policy_tests` y `ssf2t_policy_tests`,
cada uno con `work/<juego>/program.bin.data work/<juego>/program.bin`.
El helper descubre también los hooks añadidos en `games/sfz2al/src` y los de fuentes compartidas.
Para las paletas de Super Turbo: `ssf2t_colour_tests work/ssf2t/program.bin.data work/ssf2t/program.bin`.
La prueba ejecuta el cargador original y comprueba las palabras de color copiadas en VRAM.

La distribución incluye los resultados de la comprobación de los 18 juegos, la prueba
local de dos clientes en línea y las pruebas de regresión. Son pruebas automatizadas;
no sustituyen una sesión real con dos teclados físicos y un mando.

La prueba de EQ compara el DSP con los métodos originales de MAMEPPK conservados
en `mame_equalizer_oracle.h`; comprueba respuesta en frecuencia, canales, continuidad
entre bloques y la conversión SDL original frente al nuevo camino de coma flotante.
La verificación de audio usa WAVs de SFA2 con ganancias planas, Bypass y ganancias
activas, además de una banda sonora de prueba generada a 48 kHz. Los hashes del juego
coinciden aunque el EQ sea distinto. El cambio no añade campos al estado guardado.

## Distribución

Juntar los 18 ejecutables y el launcher con las DLL, licencias y shaders de la release
oficial v1.2.0 verificada. Conservar el archivo `input-lab-portable.txt` para usar `user/<id>`.
No distribuir `work/`, dumps, C++ traducido de las ROMs ni datos de juegos con las fuentes.
Conservar las licencias originales y ofrecer estas fuentes bajo GPL-3.0.
Incluir también `licenses/mame-equalizer.txt` y el `THIRD_PARTY_NOTICES.md` de estas fuentes
en la distribución binaria; contienen la atribución y condiciones del DSP adaptado.
