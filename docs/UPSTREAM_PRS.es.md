# Contribuciones al proyecto oficial

[English](UPSTREAM_PRS.md) | Español

Se publicaron seis PR separados **en borrador** en el proyecto oficial. Cada uno explica el alcance,
las pruebas realizadas, las pendientes y una captura ilustrativa del fork integrado.

| Propuesta | Dependencia compartida |
|---|---|
| [RawInput Multi Keyboard](https://github.com/tmcleroy/cps2recomp/pull/224) | recomp-common #51 |
| [Neutralize Opposing Directions](https://github.com/tmcleroy/cps2recomp/pull/225) | Independent / Independiente |
| [Physics Assignment Joysticks](https://github.com/tmcleroy/cps2recomp/pull/226) | recomp-common #51 |
| [Sliders](https://github.com/tmcleroy/cps2recomp/pull/227) | recomp-common #51 |
| [New Cheats](https://github.com/tmcleroy/cps2recomp/pull/228) | Independent / Independiente |
| [Launcher Hardening](https://github.com/tmcleroy/cps2recomp/pull/229) | recomp-common #51 |

La base compartida se propone en [recomp-common #51](https://github.com/tmcleroy/recomp-common/pull/51):
extensiones opcionales de controles, captura de ejes con signo, conflictos de binds por jugador, controles
numéricos del menú y política de tamaño inicial de ventana. Cuatro propuestas adoptan la misma copia
canónica y su manifiesto; deberán actualizar su referencia cuando upstream acepte esa API.

Pasaron las compilaciones Windows y las pruebas específicas documentadas. Siguen pendientes la batería
completa de partidas/hash/rollback, otras plataformas y nuevas sesiones físicas de aceptación en las ramas
aisladas. Borrador no significa aceptado ni integrado.

Global Bindable Hotkeys y Audio Equalizer siguen en el fork y su [galería](SCREENSHOTS.es.md), sin PR adicional
en este lote. No se incluyó DSP de MAME en estas propuestas. Launcher Hardening se limita al tamaño inicial
y ancho de contenido; no duplica el trabajo de rutas/ROMs de #208 ni desactiva el actualizador oficial.

La publicación no modifica el ejecutable instalado ni el código de juego de la build 018 ya probada.
