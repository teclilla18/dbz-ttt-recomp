# Current state

Updated 2026-11-22. Phase 0 complete; Phase 1 task 1 evaluation complete.
Destination: native Nintendo Switch OLED and Switch 2. PC is for development.

## Implemented

- Source-only safeguards and installed pre-commit/pre-push hooks; private repo.
- Recon, NID inventory, reuse survey and phased plan.
- Original synthetic probes for an external PSP-to-C candidate.
- No adopted translator/runtime, playable build or game implementation.

## Tested

- [confirmed] External sp00nznet/psprecomp pinned, unmodified, at
  `14749c1f9a94cecf55070d676ce80bc61cf49a5f`. MSVC x64 Debug build passed;
  all 27 upstream tests passed with assertions enabled, default thread mode.
- [confirmed] info/cover/funcs/emit returned zero on the main and 11 overlays.
  Baseline generated C compiled to objects in 290/296 cases. Six main C units
  failed with invalid number suffixes; all 108 overlay units compiled separately.
- [confirmed] Original synthetic probes reproduce zero-entry omission, working
  fixed-entry discovery, malformed vector-immediate C and prefix symbol collisions.
  Synthetic C compilation reproduced MSVC C2059. Probe refuses output outside
  build/ and refuses existing output directories.
- [confirmed] Main metadata is missed by the tool: zero imports instead of 228.
  Restoring three names in a separate diagnostic ELF preserves all loaded bytes;
  the parser then sees 228 imports. Discovery on that copy timed out at 90 s.
- Earlier: original/copy ISO SHA-1 matched; ULES01456 version 1.00, main entry
  0x08804040, 228 imports in 25 libraries. Eleven archive code pieces share
  0x08A81660; all 950 declared gzip streams decompressed with matching lengths.
- Earlier: 15 synthetic game-data guard checks passed. OLED remote connection,
  read-only FTP listing, screen captures and reversible HOME navigation passed;
  selection restored and remote controller detached. No app installed/launched.
- No game C executed, native game program linked, GPU/audio behaviour checked,
  Switch build produced, console deployment, gameplay or online test performed.

## Decision and limits

- Keep the C11 tool external for diagnosis. Production translator selection is
  OPEN: missing metadata/discovery, malformed C, opcode gaps and overlay naming/
  dispatch need work. Passing upstream tests is not game or CPU acceptance.
- Local trial C/object/log files remain ignored in build/evaluation/. External
  checkout/build live in a temporary tools folder outside the repository.
- No third-party source or PSP keys adopted. GPL adoption undecided; credited
  MIT/BSD/zlib reuse is permitted by AGENTS.md.
- Nothing accepted as playable by the user. Nothing released.

## Next small task

Make metadata/entry discovery reliable for this copy, isolate the named-copy
analysis stall with synthetic cases, verify all 228 imports, and prove overlay
loading before designing integration. See the evaluation note's acceptance gaps.
Continue one tested, guarded commit at a time. Switch then Switch 2 remain the
hardware targets; online waits for stable offline play.

## Where to read

- [Tool evaluation](docs/notes/tool-evaluation.md)
- [Plan](docs/notes/plan.md)
- [Recon](docs/notes/recon.md)
- [Research and credits](docs/notes/research.md)
- [System imports](docs/notes/imports.md)
