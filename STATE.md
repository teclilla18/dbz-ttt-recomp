# Current state

Updated 2026-11-22. Phase 0 only; Phase 1 has not started.

## Implemented

- Repository setup and source-only safeguards: `.gitignore`, game-data guard,
  tracked pre-commit and pre-push hooks. Hooks installed in this local checkout.
- Private GitHub repository created; it must stay private.
- Recon, NID inventory, reuse survey and a proposed Phase 1 plan.
- No recompiler, native runtime, renderer or playable build exists.

## Tested

- [confirmed] Original/copy ISO SHA-1 equal:
  `99b9075c67e17a2f99716f5d0b356e34623bd385`.
- [confirmed] SFO identifies ULES01456 version 1.00; encrypted main executable
  decrypted locally with an external tool, which stays outside the repository.
- [confirmed] Main ET_EXEC entry `0x08804040`; 228 imports / 25 libraries.
- [confirmed] Archive has 11 executable ELF members sharing `0x08A81660`.
  All 950 declared gzip members decompressed with matching corrected lengths.
- [confirmed] 15 synthetic guard checks passed, including actual hook allow/deny,
  staged-content checks and rejection of forbidden files deleted from history.
  The installed hook also passed on the staged Phase 0 source/docs.
- No PC/emulator/console boot, media playback, gameplay or online test performed.
- [confirmed] OLED connection check: remote status, read-only FTP root listing,
  three direct screen captures, HOME selection right and back verified visually.
  Original selection restored; remote controller detached. Nothing launched or
  installed. This tests the hardware connection, not the DBZ port.

## Accepted / released

- Nothing accepted as playable by the user. Nothing released.

## Main findings and limits

- [likely] Game-managed overlays require dispatch by overlay identity and address.
  Their loader and lifetime are not proved yet.
- [confirmed] Low-level ATRAC, SAS, MPEG, GE and ad-hoc functions are imported.
  Import presence does not prove execution; media subtypes remain uncertain.
- Existing PSP and same-game recompilers are research candidates, not adopted
  dependencies or proof that this European game copy runs natively.
- No third-party source or PSP keys imported. GPL adoption remains undecided;
  credited MIT/BSD/zlib reuse is permitted by updated AGENTS.md.

## Next action

Stop after presenting Phase 1. When the user starts it: evaluate an existing
C-oriented PSP tool externally, prove overlay loading and synthetic Allegrex/FPU/
VFPU correctness, then generate game C only in ignored `build/`.
PC boot comes in Phase 2; Switch, Switch 2 and online follow in that order.

## Where to read

- [Recon](docs/notes/recon.md)
- [Research and credits](docs/notes/research.md)
- [System imports](docs/notes/imports.md)
- [Plan](docs/notes/plan.md)
