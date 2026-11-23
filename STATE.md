# Current state

Updated 2026-11-22. The recompiled game runs on PC through its menus and into
the first stage's story introduction. Destination: native Switch OLED and Switch 2.

## Where it stands (tested on PC, Windows, MSVC Release)

- [confirmed] Boots: autosave notice, Spike logo, title screen, title menu
  (New Game / Load Game / Install Data), player name, save creation, and the
  Main Menu with its 3D models. Checked by scripted runs and screenshots.
- [confirmed] Overlay swaps work live: 1 -> 3 -> 11 -> 4, matching overlays.md.
- [confirmed] A save file is written under build/fs/ms/PSP/SAVEDATA/.
- [confirmed] The first-stage loading stall is fixed: the same fresh-save
  route now reaches story pages, advances scripted dialogue, and continues
  beyond frame 25797 at the 200-second watchdog. Tested on PC, MSVC Release
  with guest tracing enabled; story captures inspected at 9000 and 11400.
- [confirmed] Cause: VFPU branches always became false in emitted C; `vcst`
  also used the wrong constant-selector field. Both corrected. Synthetic
  generated-C execution tests fail before the fixes and pass afterward.
- Playable combat and completion of a battle remain unverified.
- Not accepted by the user; nothing released. No Switch build yet.

## How it is built

- Translator and runtime: psprecomp v0.6.0 (MIT), copied into
  third_party/psprecomp with our changes listed in its UPSTREAM.md. Decryption
  code removed from the copy. Toolkit tests: 27/27 pass after the corrections;
  the new branch execution test covers 3072 cases.
- tools/build_game.py: generates C for main + 11 overlays into build/gen/
  (function-start hints, overlay symbol prefixes, overlay table, all 228
  firmware stubs). games/dbz/: host program and overlay swap hooks.
- tools/extract_disc.py copies disc files to build/fs/disc; the game reads its
  archive through the raw disc device backed by baserom/game.iso.
- Project tests: 17 Python tests pass (metadata, scanner, NID names).

## Known gaps (implemented as placeholders, not real)

- Movies (sceMpeg) are skipped; music (ATRAC3+) decodes as silence; PCM sas
  voices are silent; no sound reaches the speakers yet (mixing only).
- Ad hoc networking reports "not connected"; WLAN switch reads off.
- Save data: load/save/size handled; list/delete modes only return OK.
- Text entry returns a fixed name (PSP_PLAYER_NAME, default "Player").
- Rendering is psprecomp's software renderer in a Win32 window.
- Known minor: an early read from address 0 (32 bytes) is logged at boot.

## How to run (PC)

1. `python tools/extract_disc.py` then `python tools/build_game.py`
2. Configure/build with CMake (Visual Studio 2022), target dbz_ttt.
3. `tools/run_pc.ps1 -Seconds 60`. Keys: arrows, Z=Cross, X=Circle,
   A=Square, S=Triangle, Q/W=L/R, Enter=Start; Xbox pad works.
   Test switches: DBZ_SHOTS=<dir>, DBZ_PRESS="frame:button,...",
   PSPRECOMP_TRACE_HLE=N, PSPRECOMP_TRACE_IO=1, DBZ_LOG_OVERLAYS=1,
   DBZ_DUMP_SEC=160,200 (timed thread/call/entry/loop snapshots).
   Configure with -DDBZ_TRACE=ON to enable guest entry/back-edge tracing;
   the default is OFF. Diagnostic runs use separate filesystem/save roots.

## Next small tasks

1. Continue the first stage's story introduction and reach a playable battle
   on PC. The prior loading stall is resolved; see battle-load.md.
2. Test a battle through completion; then sound output and music decoding.
3. Switch: port the window/input/audio layer to libnx (devkitA64 is
   installed), build an NRO, test on the OLED.
   Required controls: menus use Switch A-confirm/B-cancel; combat in every
   mode uses Xbox positions (B=Cross, A=Circle, Y=Square, X=Triangle).
   This is recorded only; see controls.md for context/transition checks.

## Where to read

- [Overlay rules](docs/notes/overlays.md), [Plan](docs/notes/plan.md),
  [Metadata](docs/notes/metadata.md), [Research](docs/notes/research.md),
  [Recon](docs/notes/recon.md), [Imports](docs/notes/imports.md)
- [Stage loading diagnosis and reproduction](docs/notes/battle-load.md)
- [OLED menu and battle controls requirement](docs/notes/controls.md)
