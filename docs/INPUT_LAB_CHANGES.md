# Diferencias funcionales de Input Lab 018

Base comparada: CPS2Recomp **1.2.0**, commit oficial
[`f1212bca038f543e30fb106d8f88dabf30374df5`](https://github.com/tmcleroy/cps2recomp/commit/f1212bca038f543e30fb106d8f88dabf30374df5).
Versión del fork: `1.2.0-inputlab.018`. Las novedades oficiales posteriores se documentan por separado
en [UPSTREAM_DIFFERENCES.md](UPSTREAM_DIFFERENCES.md), sin atribuirlas a esta build.

## Funciones heredadas de la versión oficial

Los dieciocho juegos, recompiladores 68000/Z80, modelo de placa, renderizado GPU/SDL, shaders,
widescreen, zoom, modos de entrenamiento y alternativos, editor de color, bandas sonoras, Multi game,
lobby, chat y rollback proceden del proyecto oficial. Ya existían configuración de botones, layouts
compartidos/propios, opciones de personajes ocultos y rapid fire. Input Lab extiende o sustituye partes
de esos sistemas; no reclama la autoría de las funciones originales.

## Teclados físicos independientes (Windows)

La entrada convencional combina los teclados del escritorio. Se añade RawInput y hasta cuatro slots:
cada interfaz conserva sus propias teclas físicas, teclas bloqueadas y conexión. Sus estados se combinan
con OR solamente entre interfaces del mismo teclado físico. Soltar W en P1 no suelta W en P2; una interfaz
auxiliar tampoco borra lo mantenido por otra interfaz de su propio grupo.

La identidad física usa el contenedor PnP de Windows. Coincidir en nombre, VID/PID o modelo no basta
para agrupar dos dispositivos. Los contenedores nulos/del sistema no son prueba de identidad; si no se
puede justificar el grupo, se conserva la identificación independiente. No se codifican los números
de interfaz de la máquina de desarrollo. Las interfaces de teclado de un ratón no se mezclan por
compartir teclas con los jugadores.

En **Controllers → Use separate keyboards** se asigna un teclado por jugador y se observa **Keys held**.
Se conservan asignaciones al reconectar/reiniciar; conectar otro teclado no reescribe los binds.
**P2 uses Player 1's layout** reutiliza esquema y opciones sin borrar el perfil propio de P2.
Editar P2 ya no elimina las mismas teclas de P1, y viceversa. En modo separado los atajos globales
pertenecen a P1; ESC abre el menú desde cualquier teclado.

RawInput separado requiere Windows. Esta corrección evita mezclas de estado entre dispositivos;
no supera el ghosting/rollover eléctrico de cada teclado. La captura de diagnóstico es optativa:
el uso normal no requiere logger, cuyos retrasos de observación no prueban por sí solos pérdida de teclas.

Fuentes: [`port/keyboards.h`](../port/keyboards.h), [`port/raw_keyboards.h`](../port/raw_keyboards.h),
[`launcher/src/keyboard_controls.cpp`](../launcher/src/keyboard_controls.cpp), frontend y bindings.

## Direcciones, combinaciones y autofire

| Función | Comportamiento y ámbito |
|---|---|
| Neutralize Opposing Directions (MAME) | Por juego/jugador. L+R = neutral horizontal; U+D = neutral vertical. L+D+R conserva D. Se aplica al estado final de entrada. |
| x3 punches / x3 kicks | Acciones PPP/KKK simultáneas, asignables por teclado o mando. Se integran con las existentes sin duplicarlas. |
| Custom 1–4 | Cuatro máscaras de los seis botones por jugador. Los seleccionados se presionan a la vez: no son secuencias temporizadas ni reproducción de movimientos. |
| Autofire | Por acción: seis botones, PPP, KKK y cuatro Custom. Demora 1–30 frames encendidos/apagados; primera pulsación inmediata, reinicio al soltar. |

El autofire sigue frames del juego, también en cámara lenta; no avanza nuevamente en frames especulativos
de run-ahead. Hay un solo procesador por jugador, evitando duplicar la función entre rutas de entrada.
El rapid fire oficial se integra en este sistema (demora 2 y selección del primer botón o los seis)
en vez de mantener una segunda implementación paralela.

Los perfiles son individuales por juego, aunque se comparta el layout de binds: otro juego no hereda
sus máscaras Custom, autofire o neutralización. Fuentes: [`port/input_profile.h`](../port/input_profile.h),
[`launcher/src/input_controls.cpp`](../launcher/src/input_controls.cpp), [`port/menu.cpp`](../port/menu.cpp).

## Mandos y atajos globales

La asignación física de un mando a un jugador es independiente de su posición de enumeración SDL.
Un solo mando puede controlar P2. Se admiten botones, gatillos y sentidos de ambos sticks como binds,
conservando el signo de los ejes; las entradas pueden borrarse. Permanecen los layouts compartidos/propios.

Los atajos de teclado **While playing** son globales, configurables y admiten modificadores.
ESC permanece fijo. **Reset game** reemplaza la antigua segunda entrada para abrir el menú y tiene
su propia acción. Pausa, avance de frame, velocidades, reinicio de entrenamiento, save/load, slots,
test/service, fullscreen, chat y zoom pueden configurarse sin cambiar los binds del juego.

**Gamepad — Global shortcuts** permite cambiar o limpiar Guide, clic del stick izquierdo y Start+Select
por separado. También son editables pausa/chat, avance de frame, entrenamiento, zoom y Multi.
Admite hasta tres entradas simultáneas, incluidos gatillos y sentidos de stick. **Clear all gamepad
shortcuts / Restore gamepad shortcuts** solo actúan sobre los atajos del mando. Una entrada borrada
se guarda como `none` y no reaparece al reiniciar.

Se resuelven conflictos en el mismo contexto: pausa offline y chat online pueden compartir entrada.
Las combinaciones tienen prioridad; las entradas individuales que también sean del juego conservan
esa función. Si menú y cámara comparten botón, el toque abre el menú y usar cámara cancela el toque.

Fuentes: [`port/hotkeys.h`](../port/hotkeys.h), [`port/pad_hotkeys.h`](../port/pad_hotkeys.h),
[`launcher/src/global_controls.cpp`](../launcher/src/global_controls.cpp), cambios en `recomp-common/launcher/src/bindings.h`.

## Ecualizador por juego

**Game → Sound → Equalizer**: DSP paramétrico de cinco bandas adaptado del ecualizador de
MAMEPPK/MAME de Olivier Galibert. Se conserva su [licencia BSD-3-Clause](../licenses/mame-equalizer.txt).

| Banda | Frecuencia inicial | Rango Hz | Modo |
|---|---:|---:|---|
| Low | 100 | 20–2000 | Shelf / Peak |
| Low Mid | 330 | 100–10000 | Peak |
| Mid | 1000 | 100–10000 | Peak |
| High Mid | 3300 | 100–10000 | Peak |
| High | 10000 | 500–16000 | Shelf / Peak |

Ganancia −12 a +12 dB (pasos 0.1); Q 0.10–10.00 en Peak (inicial 0.7071067).
Mode Active/Bypass; inicialmente plano, 0 dB. Izquierda/derecha ajustan, Shift acelera, Enter restaura
el slider y Reset All restaura solo el EQ del juego. El menú mantiene la pausa: se escucha al reanudar.

Se filtran en coma flotante el sonido de placa y la música personalizada, con estado independiente
por canal/fuente y límite de Nyquist. Los filtros se reinician al cargar estados, resetear o cambiar
parámetros/frecuencia. El EQ no entra en hashes ni estados: cargar una partida no reemplaza la
ecualización elegida. Valores en `[equalizer]` del `settings.ini` de cada juego.

Fuentes: [`port/equalizer.h`](../port/equalizer.h), frontend de audio y menú.

## Sliders de temporización e imagen

Menú **Slider controls**, con valores exclusivos por juego:

| Control | Rango | Significado |
|---|---|---|
| Overclock CPU :maincpu | 25–400 %, paso 0.1 | Trabajo disponible del procesador principal. |
| Overclock CPU :audiocpu | 25–400 %, paso 0.1 | Reloj de CPU de audio; no equivale al reloj QSound ni cambia por sí solo la tasa de muestras. |
| Screen Refresh Rate | 40–90 Hz, paso 0.1 | Frecuencia de video de la placa virtual; no modifica el modo del monitor. |
| Screen Brightness / Contrast | 0–2, paso 0.01 | Transformación local de imagen. |
| Screen Gamma | 0.1–3, paso 0.01 | Corrección local de tonos medios. |

El refresco inicial CPS2 es `8000000 / 512 / 262` Hz. Frecuencia de video, reloj de CPU y velocidad de
emulación son distintos. El audio adicional de Definitive conserva su temporización propia.
Estados antiguos con backend driver pueden necesitar Reset game para aplicar el reloj de audio.
En red se usan relojes originales y se conservan los valores offline. Los ajustes de imagen no cambian
RAM de video, estados ni hashes, y el menú continúa legible. Fuentes: [`port/tuning.h`](../port/tuning.h),
`machine/machine.*`, frontend y menú.

## Cheats por juego

Las opciones están en **Cheats** y usan `kFeatureOwn`: no se propagan como opciones globales de colección.
Se conservan los cheats oficiales. Los sitios de instrucciones se verificaron en estas revisiones base;
no se afirma validación exhaustiva de cada región alternativa ni se aplican offsets de otra revisión sin comprobarlos.

### Damage hits

| Juego | ROM base validada | Rutas contempladas |
|---|---|---|
| Street Fighter Alpha | `sfa` Europe 950727 | Daño normal, agarres, throws y propiedad original del tercer luchador. |
| Street Fighter Alpha 2 | `sfa2u` USA 960430 | Implementación propia previa: rutas normales/alternativas, throws y holds. |
| Street Fighter Zero 2 Alpha | `sfz2al` Asia 960826 | Tercer luchador de Dramatic Battle, proyectiles/ayudantes. Al activarlo se ejecuta la rutina original, sustituyendo solo las lecturas de rango/clase. |
| Super Street Fighter II Turbo | `ssf2xjr1` Japan 940223 | Rangos de ataque/defensa en golpes, proyectiles y throws. |

- **Vanilla**: cálculo original.
- **Equal to human**: ambos lados usan el perfil humano-atacando-CPU actual.
- **Equal to CPU**: ambos usan el perfil CPU-atacando-humano actual, sin forzar el máximo.

El rango depende de dificultad y adaptación durante la partida; el de CPU no siempre es mayor.
Se conservan modificadores de personaje, movimiento, bloqueo, vida, combo y azar. No se cambia la IA,
propiedad del luchador ni sus rangos almacenados. PvP normal y CPU vs CPU conservan Vanilla.
Igualar el perfil no significa que todos los personajes o movimientos quiten la misma vida.

Fuentes: [`runtime/damage_policy.h`](../runtime/damage_policy.h), `games/sfa/src/inputlab_damage.cpp`,
`games/sfa2/src/damage.cpp`, `games/sfz2al/src/inputlab_damage.cpp`, `games/ssf2t/src/inputlab_cheats.cpp`.

### Super Turbo: P1 / P2 Select Character

Off, dieciséis personajes normales, Akuma/Gouki seleccionable y **Shin Akuma/Gouki (CPU boss version)**.
Elegir antes de confirmar el personaje; un cambio durante la pelea afecta a la próxima inicialización.
Off deja de forzar la selección, no convierte retroactivamente al personaje activo.

El puente del jefe usa recursos originales de ataques/animación, Tatsumaki, teleport, doble proyectil
aéreo y trayectoria, manteniendo comandos humanos. Ajusta la entrega original del hitstop del atacante
en cancelaciones normales→especiales aceptadas. No cambia hitboxes, separación, límites de juggle,
IA, propiedad, rango de daño ni rival de la escalera; Ken no se fuerza. Las pruebas comparan rutinas CPU
originales con el puente humano. No se promete que cualquier combo imaginado sea realizable ni se afirma
haber cubierto todas las secuencias de combate posibles.

### Super Turbo: Select Speed

Off; **No frameskip (like SSF2)**; **Turbo 1–4 (Japan)**; **Maximum frameskip (double speed)**.
Cambia el operando de la comparación original del frameskip, sin sobrescribir la elección guardada
de velocidad de la placa/jugador. Off vuelve a usarla inmediatamente. Es independiente de CPU, Hz,
VSync y velocidad de emulación; no interpola imágenes nuevas.

### Super Turbo: P1 / P2 Select Colour

Off y diez opciones comunes de MAME. Fuerza la paleta asociada a la opción del selector;
no vuelve a bindear el botón físico de confirmación.

| Opción | Akuma y Shin Akuma |
|---|---|
| LP / MP / HP button | Negro original (0A) |
| LK / MK / HK button | Gris oscuro original (0B) |
| Start button / Hold any P/K button | Gris oscuro original (0B) |
| SSF2 primary / SSF2 alternate | Negro / gris oscuro |

Los dieciséis personajes normales mantienen sus diez paletas. Se aplica antes de que el cargador original
copie los colores a VRAM, en la próxima ronda. El retrato de Akuma sigue negro en la selección.
Off deja de forzar el color y conserva la elección que tenga el juego. No se crean paletas nuevas.
La correspondencia también se corrigió en el cheat MAMEPPK externo; ese XML y el proyecto MAME no forman parte de este repo.

### Hidden characters: selección con START

La base oficial ya tenía opciones de personajes ocultos. En los tres juegos originales cambian
el método y la descripción, y la opción queda en Cheats:

- **SFA2**: START sobre Ryu, Akuma, Chun-Li, Dhalsim o Zangief alterna su forma oculta correspondiente
  (Evil Ryu, Shin Akuma y variantes clásicas), para P1, P2 o ambos.
- **Vampire Savior**: START sobre Gallon alterna Dark Gallon; sobre Bishamon, Oboro Bishamon.
  Se conserva Shadow del cuadro `?` y el comportamiento de colores del juego.
- **Puzzle Fighter**: START cicla normal → Devilot → Akuma → Dan → normal. Mover el cursor restaura
  el personaje normal; los dos botones confirman sus dos colores. Los códigos originales funcionan con el cheat apagado.

Las otras familias conservan sus métodos oficiales. Las funciones específicas de SFA2 se compilan con
su definición propia para no imponerlas a Alpha 3/SFZ2AL por compartir fuentes.

## Launcher, persistencia y compatibilidad

- Ventana inicial centrada y más amplia, con márgenes; no maximizada.
- Filas Custom ajustadas al panel y controles separados del bloque de atajos globales.
- Modo portátil Input Lab `user/<id>` junto al formato oficial; perfiles personales excluidos del repo.
- Logs normales separados por juego; captura detallada de entrada optativa.
- Importación de estados propios antiguos, distinguiendo v6 propia de v6 oficial; estados nuevos v9.
  Se importa la placa, no los metadatos auxiliares obsoletos del entrenamiento antiguo.
- Se reconocen estados antiguos junto a `settings.ini` en Windows.
- Versión Input Lab distinta para evitar emparejar binarios de red incompatibles.
- Se mantienen las reglas oficiales de cheats online y los relojes originales en red.
- Actualizador oficial desactivado: no debe reemplazar los agregados por un binario stock.
- Manifiesto Windows y rutas Unicode; librería estática común opcional para compilar la colección,
  helper de reconstrucción, entradas de traducción y pruebas adicionales.

## Validación, publicación y créditos

Se publica el código de juego de la build 018 confirmada por el usuario, con documentación y `.gitignore`
adicionales. Validación local: 18 builds/arranques; seis suites de regresión; 10 801 comprobaciones SFA,
746 874 SFA2, 19 441 SFZ2AL, 12 457 Super Turbo; 396 casos/32 475 comprobaciones de paleta en el port;
60 casos de paleta MAME; partida nativa Shin gris vs Akuma negro y menú guardado/recargado.
El EQ se contrastó con el oracle MAME y fixtures de audio en la build 016.

Se publican fuentes, pruebas, direcciones de entrada (no instrucciones traducidas), licencias y resúmenes.
Se excluyen ROMs, claves, programas traducidos/descifrados, builds locales, logs de teclados,
identificadores físicos reales, partidas y ajustes personales. GPL-3.0-only; DSP EQ BSD-3-Clause con atribución.
Validado en Windows x64; no se afirma que el CI del fork haya pasado ni que se hayan probado estas
modificaciones en macOS, Linux o Android. Los workflows oficiales se conservan sin adaptar; su comprobación
de `recomp-common` detectará las modificaciones intencionadas de esta rama.
