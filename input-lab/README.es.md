[English](README.md) | Español

# Input Lab 018

Base oficial: CPS2Recomp v1.2.0, commit `f1212bca038f543e30fb106d8f88dabf30374df5`.
Versión personalizada: `1.2.0-inputlab.018`.

## Nuevo en la 018: Select Colour por jugador en Super Turbo

**Super Street Fighter II Turbo (ssf2xjr1, Japan 940223) → Game → Cheats → P1 / P2 Select Colour**.
Cada jugador tiene Off y las diez opciones habituales de MAMEPPK; inicialmente Off.
La elección fuerza la paleta correspondiente al botón indicado, aunque se confirme el personaje con otro botón.
Se aplica cuando el juego carga los colores de la ronda; elegirla antes de empezar la próxima ronda.

| Opción del selector | Akuma y Shin Akuma |
|---|---|
| LP / MP / HP button | Negro |
| LK / MK / HK button | Gris oscuro |
| Start button / Hold any P/K button | Gris oscuro |
| SSF2 primary / SSF2 alternate | Negro / Gris oscuro |

Los demás personajes conservan sus diez paletas originales. Off conserva la selección original del juego.
La pantalla de selección sigue mostrando el retrato negro de Akuma: el resultado se aprecia al entrar en la pelea.
Se usan exclusivamente las dos paletas originales de Akuma, sin modificar gráficos ni reglas de combate.
El ajuste se guarda por jugador y por juego, también accesible en la página del juego del launcher.

La misma correspondencia ya está instalada en `0.289-sliders/cheat/ssf2t.xml` de MAMEPPK.
Se retiraron las dos entradas exclusivas negro/gris de la 017: ahora se eligen las opciones comunes.
Una elección guardada como negro se migró a LP; una gris oscuro, a LK. Los demás cheats y ajustes se conservaron.
Reiniciar MAME para que lea el XML nuevo y elegir el color antes de la siguiente ronda.

Pruebas: 60 casos de MAME contra sus paletas originales en VRAM; 396 casos de CPS2Recomp
para todos los personajes, ambos jugadores y Off, con 32 475 comprobaciones. Partida nativa
Shin Akuma gris contra Akuma negro, guardado/recarga del menú y 12 457 comprobaciones de
regresión del jefe/daño/velocidad. Con los colores en Off, 2 100 imágenes coinciden con la 017.
Los ajustes actuales de la 017 se copian con respaldo en `backups/build_017_user`; la 017 permanece intacta.

## Nuevo en la 017: cheats de daño y Super Turbo

Durante la partida: **Game → Cheats**. También aparecen en la página propia
del juego en el launcher. Los valores son exclusivos de ese juego.

- **Damage hits**: Vanilla / Equal to human / Equal to CPU en SFA (`sfa`, Europe 950727),
  SFA2 (`sfa2u`, USA 960430), SFZ2AL (`sfz2al`, Asia 960826) y SSF2X (`ssf2xjr1`, Japan 940223).
  Se conserva el ajuste previo de SFA2. SFZ2AL incluye su tercer luchador en Dramatic Battle.
- El daño usa el perfil actual, con la dificultad y el rango adaptativo del juego.
  Conserva los modificadores propios de personaje, movimiento, bloqueo, vida, combo y azar.
  No fuerza el rango máximo ni cambia quién controla un luchador. PvP normal y CPU vs CPU conservan Vanilla.
- **P1 / P2 Select Character** en Super Turbo permite seleccionar también Akuma normal y
  **Shin Akuma / Gouki (CPU boss version)**. Elegirlo antes de confirmar personaje;
  cambiarlo durante una pelea tendrá efecto en la siguiente inicialización de personaje.
  Off devuelve la selección normal para la próxima inicialización.
- El jefe usa sus recursos originales, doble proyectil aéreo, teleport, Tatsumaki y
  transición de golpes normales a especiales mediante comandos humanos. No se alteran
  cajas de impacto, separación, reglas de juggle ni rango de daño.
- **Select Speed** en Super Turbo: Off, No frameskip (like SSF2), Turbo 1–4 (Japan) y
  Maximum frameskip (double speed). Es el frameskip interno de la placa, independiente
  del overclock, velocidad de emulación o Hz de pantalla. Off recupera inmediatamente
  la elección de la placa/jugador; no se sobrescribe esa elección.

Se mantienen el ecualizador, teclados separados y sus grupos de interfaces, controles
de mando, neutralización, macros, atajos y demás funciones de la 016.
Los ajustes se copian desde la build 016 y se respaldan en `backups/build_016_user`.
La build 016 permanece intacta. Las opciones nuevas comienzan desactivadas.

Pruebas: 10 801 comprobaciones SFA, 19 441 SFZ2AL, 12 457 SSF2X y 746 874 SFA2,
incluidas comparaciones de instrucciones/flags/ciclos/RAM con el 68000 original,
88 pares de inicializaciones del jefe CPU frente al puente humano y 2 520 casos de cancelación.
Los cuatro juegos con cheats apagados coinciden con la 016 en 2 100 imágenes consecutivas.
Se incluyen capturas del menú real y una partida nativa Shin Akuma vs Ken en `validation/cheats-017`.
Estas comprobaciones no prueban todas las combinaciones de combate posibles.

## Nuevo en la 016: Equalizer por juego

Abrir el menú durante la partida y entrar en **Game → Sound → Equalizer**.
El DSP adapta el ecualizador paramétrico de MAMEPPK / MAME 0.289 de Olivier Galibert.

| Banda | Frecuencia inicial | Rango | Modo |
|---|---:|---:|---|
| Low Band | 100 Hz | 20–2000 Hz | Shelf / Peak |
| Low Mid Band | 330 Hz | 100–10000 Hz | Peak |
| Mid Band | 1000 Hz | 100–10000 Hz | Peak |
| High Mid Band | 3300 Hz | 100–10000 Hz | Peak |
| High Band | 10000 Hz | 500–16000 Hz | Shelf / Peak |

- Mode: Active o Bypass. Inicialmente Active con todas las ganancias a 0 dB.
- Ganancia: −12 a +12 dB, en pasos de 0.1 dB.
- Q factor: 0.10 a 10.00, inicialmente 0.7071067 (se muestra 0.71). Aparece en modo Peak.
- Izquierda/derecha ajustan; Shift acelera el ajuste; Enter restaura el valor de ese slider.
- Las frecuencias usan la escala de pasos de MAME. Los encabezados de bandas se saltan al navegar.
- Reset All restaura solamente el EQ de este juego. Se conservan controles, cheats, sliders y los demás juegos.
- El menú mantiene su pausa habitual: los cambios se escuchan al reanudar la partida.

Los valores se guardan en `[equalizer]` dentro de `user/<juego>/settings.ini`.
Se aplican a la salida de la placa y a la banda sonora personalizada, con filtros
independientes por canal y fuente. Las frecuencias efectivas respetan el límite
de Nyquist de la fuente (la placa CPS2 produce audio a 24038 Hz; las bandas sonoras pueden tener otra frecuencia).
Los estados del filtro se reinician al cargar una partida, reiniciar la placa,
cambiar parámetros o cambiar la frecuencia de una fuente.

El audio en vivo se filtra en coma flotante antes de entregarlo a SDL. El WAV de
diagnóstico se limita al rango del archivo de 16 bits al exportarlo. El EQ queda
fuera de la emulación, los estados guardados y los hashes de red; cargar un estado
no reemplaza el EQ elegido en los ajustes del juego.

Los ajustes originales están respaldados en `backups/build_015_user`; la build 015 permanece intacta.
Las fuentes y binarios conservan la licencia y atribución BSD-3-Clause del DSP original.

## Nuevo en la 015: atajos del mando editables

En Controllers, después de los atajos de teclado, aparece **Gamepad - Global shortcuts**.
Guide, clic del stick izquierdo y Start+Select se pueden cambiar o borrar por separado.
Para eliminar todos los accesos del mando al menú, borrar sus tres filas **Open / close menu**.
**Clear all gamepad shortcuts** borra todos los atajos del mando; no borra los controles del juego ni los del teclado.
Se puede restaurar solamente el conjunto de atajos del mando con **Restore gamepad shortcuts**.

Hacer clic en una fila, mantener un botón o una combinación de hasta tres entradas y soltar.
Se admiten botones, gatillos y sentidos de ambos sticks. Clear o Retroceso/Supr borran la entrada; ESC cancela.
Los valores se guardan globalmente en `[gamepad_hotkeys]` dentro de `user/hotkeys.ini`.
Una asignación vacía se guarda como `none` y permanece vacía al reiniciar.
Al reutilizar una combinación se elimina el atajo que entre en conflicto en el mismo contexto;
por ejemplo, asignar Guide a Reset quita su acceso anterior al menú. Se conservan las parejas
que tienen contextos distintos, como pausa fuera de línea y chat en línea.
Las entradas individuales que también sean controles del juego conservan su función del juego;
las combinaciones tienen prioridad. Los atajos se aplican a los mandos asignados a jugadores.

Los accesos de pausa, chat, avance de frame, cámara y Multi game también son editables.
Sus contextos se conservan: pausa fuera de línea; chat en línea; avance de frame mientras está pausado.
Si menú y cámara comparten un botón, un toque abre el menú y usar la cámara cancela ese toque.
Se conserva ESC fijo como acceso por teclado.

Se conserva el código nuevo de los 18 juegos, sus correcciones, modos de juego,
renderizado GPU, shaders, zoom, bandas sonoras, juego en línea, sala y colección.
Los cambios propios se integraron sobre esa base, sin copiar encima el launcher antiguo.

## Funciones propias conservadas

| Función | Ámbito y tratamiento |
|---|---|
| Teclados independientes | RawInput por interfaz; OR solamente entre interfaces del mismo dispositivo físico; asignación persistente. Cuatro slots para los juegos nuevos. |
| Neutralize Opposing Directions (MAME) | Por juego y jugador; izquierda+derecha y arriba+abajo se cancelan. |
| Mandos | Asignación física independiente del número de enumeración; sticks de ambos lados y medios ejes con signo. Un solo mando puede asignarse a P2. |
| PPP / KKK | Un solo par de acciones; conserva los nombres de los ajustes anteriores. |
| Autofire | Una sola implementación por jugador, con selección de botones/combinaciones y demora de 1–30 frames. Para reproducir la opción oficial: demora 2, activar el primer botón o los seis. |
| Custom | Cuatro combinaciones simultáneas por jugador, por juego. |
| Atajos | Globales, configurables, ESC fijo; Reset game conservado. Ctrl+T abre el chat sin quitar T del esquema de lucha; también se pueden cambiar los atajos de zoom. |
| Sliders | CPU principal, CPU de audio, frecuencia de video, brillo, contraste y gamma por juego. El procesador/chip de audio adicional de Definitive conserva su temporización propia. |
| Hidden characters | Los tres juegos originales conservan su selección con START y su categoría Cheats. Las nuevas familias conservan sus implementaciones oficiales. |
| Damage hits | SFA2 USA 960430: Vanilla / Equal to human / Equal to CPU, con el rango real y variable de la CPU; sin modificar su IA. |

El launcher conserva las dos opciones oficiales: layout compartido y layout propio.
Los tres juegos de la build 013 se migran a layout propio para conservar sus binds.
La edición de un layout ya no elimina las mismas teclas del otro jugador.
Los perfiles de autofire, Custom y neutralización siempre son de un juego concreto.

## Uso y datos

- Abrir `Jugar.cmd` o `cps2recomp-launcher.exe`. No se activa captura de teclado normalmente.
- Los ajustes se guardan en `user`, con una subcarpeta por juego. Dispositivos y atajos son globales.
- Los ajustes originales están respaldados en `backups/build_013_user`, `backups/build_014_user` y `backups/build_015_user`.
- Las builds 013, 014 y 015 y sus archivos permanecen intactos.
- Se conserva el formato portátil anterior, además del formato portátil oficial.
- Los estados antiguos propios v6 se distinguen de los oficiales v6 por su cabecera.
  Se importa el estado completo de la placa; los metadatos auxiliares del sistema antiguo
  de entrenamiento no se usan en el sistema nuevo. Los estados nuevos usan v9.
- Se reconocen los estados antiguos guardados junto a `settings.ini` en Windows.

## Juego en línea

Los dos extremos deben usar esta misma build. Su identificador evita mezclarla
accidentalmente con binarios oficiales diferentes. Se conservan el protocolo y los
servicios oficiales de sala, códigos, chat y rollback.

En línea se usan los relojes originales; los valores personales de overclock y refresco
se conservan para jugar fuera de línea. La imagen sigue admitiendo ajustes locales.
Los cheats siguen las reglas oficiales para partidas en línea.

El instalador automático de versiones oficiales está desactivado en esta variante:
reemplazaría las funciones propias. Las actualizaciones futuras requieren otra integración.

## Código nativo y fuentes

`entries/` contiene solamente direcciones, no datos ni instrucciones de las ROMs.
La cobertura parte de las tablas públicas de la distribución oficial verificada y de
las entradas locales previas. Todo el C++ traducido se vuelve a generar con las ROMs
y las herramientas actuales del proyecto; no se reutiliza código ejecutable oficial.

El ZIP de fuentes incluye los archivos completos modificados y las pruebas. El parche
se aplica sobre el commit oficial indicado arriba. No incluye ROMs, dumps descifrados,
código C++ generado de los juegos ni carpetas de compilación.

`BUILD.md` describe cómo reconstruir la colección. Las dependencias y sus licencias
se conservan en la distribución; las licencias de LLVM/MinGW se incluyen también.
