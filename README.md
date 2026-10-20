<img width="1309" height="132" alt="Screenshot 2026-10-16 at 9 24 00 AM" src="https://github.com/user-attachments/assets/181fa560-8f13-453d-9398-accc68d566b0" />

# DBrain-SYN-mod

Mods para **Elektron Digitakt mk1, OS 1.54**, hechos con [elekloader](https://github.com/irpina/elekloader).
Autor: **Heal Youth**.

## Firmware actual: DB09

| Mod | Versión | Autor |
|---|---|---|
| core | 4.0 | elekloader |
| seq-events | 0.1 | elekloader |
| machine-pages | 1.2 | elekloader (versión de digi1_mods) |
| **drumbrain** | **0.7** | **Heal Youth** |
| **qretrig** | **0.2** | **Heal Youth** |
| mix-collage (bootanim) | 1.0 | Heal Youth |
| Digi Mono | 0.24 | [gdeo607/digi1_mods](https://github.com/gdeo607/digi1_mods) |
| Digi Macro | 1.2 | [gdeo607/digi1_mods](https://github.com/gdeo607/digi1_mods) |
| DH-101 | 0.25 | Robbie, en [gdeo607/digi1_mods](https://github.com/gdeo607/digi1_mods) |

RAM: 209 KB de 256 KB.

## DrumBrain (`drumbrain` 0.7)

Ritmos generativos en el Digitakt, portados del dispositivo Max for Live
[DrumBrain](https://github.com/mganss/DrumBrain) (modelo de predicción por contexto, estilo Lempel-Ziv). Sin flam.

- **Dos máquinas SRC:** **DB HOUSE** (41) y **DB BREAKS** (42). Se eligen con [FUNC] + [SRC].
- **Instrumento fijo por pista:** T1 BD, T2 SD, T3 CP, T4 RS, T5 CH, T6 OH, T7 TOM, T8 CY.
  El sample suena como ONESHOT.
- **Regenerar:** [TRK] + el mismo [TRIG 1-8] dos veces (cartel "NEW: Tn"); [TRK] + [FUNC] regenera todas.
  El cambio entra al inicio de la próxima vuelta de la pista.
- Las pistas DB ignoran las condiciones y la probabilidad de los pasos guardados.
- El pattern guardado nunca se modifica.
- **Problema conocido:** con una máquina DB, la vista del sample ([SRC] dos veces) no dibuja la forma de onda
  ni las marcas de start y end.

## QRETRIG (`qretrig` 0.2)

Retrig cuantizado sobre la pista seleccionada.

- Con REC (grilla) apagado, mantener **[TRIG 14] = 1/4**, **[TRIG 15] = 1/8**, **[TRIG 16] = 1/16**.
  Las repeticiones caen en la grilla del secuenciador.
- LEDs de las teclas 14-16 en azul brillante; la tecla mantenida en blanco.
- Con REC encendido, o con [FUNC], las teclas funcionan como siempre.
- Lo más rápido es 1/16 (un paso).


<img width="714" height="342" alt="HealYouth-dinsynktak MOD-qretrig-DNT07" src="https://github.com/user-attachments/assets/2fcc6dba-4698-44dd-832e-6e8cbdd62358" />

## Contenido

- `drumbrain/`: fuentes del mod. `genmodel.py` convierte el `model.js` de DrumBrain en `dbmodel.c`.
- `qretrig/`: fuentes del mod. `qr_led.s` engancha el setter de LEDs del OS 1.54 (0x400e691c).
- `elemods/`: los dos mods compilados para OS 1.54, listos para la app de elekloader.
- `licenses/`: licencia de DrumBrain.

Los `.syx` armados no se incluyen: contienen el firmware de Elektron. Cada uno arma el suyo en elekloader
con su propio `Digitakt_OS1.54.syx` oficial.

## Pruebas

Probado en [digiemu](https://github.com/irpina/digiemu) con el firmware 1.54 real:
DB HOUSE suena en todas las vueltas, retrig 1/16 y 1/4 en los pasos correctos, sin interferir con REC
encendido, y LEDs de las teclas 14-16 en azul. DB07 confirmado en el equipo.

## Créditos y licencias

- **drumbrain** y **qretrig**: Heal Youth, GPL-2.0-or-later.
- **Modelo de DrumBrain** (las tablas de `dbmodel.c`): Copyright (c) 2019 Michael Ganss, licencia MIT
  (`licenses/DrumBrain-MIT.txt`). Original: https://github.com/mganss/DrumBrain
- **elekloader** y **digiemu**: https://github.com/irpina
- **Digi Mono, Digi Macro, DH-101**: https://github.com/gdeo607/digi1_mods

<img width="512" height="256" alt="preview_mix-collage" src="https://github.com/user-attachments/assets/6fb493f7-930b-46a5-b2e5-d032d8aea3bf" />

Mods no oficiales, sin relación con Elektron. Usar bajo tu propio riesgo.
