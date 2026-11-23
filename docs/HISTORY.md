# History

## 2026-11-22: Phase 1 task 2b, overlay loading rules (Claude Code)

- Added original tools/mips_scan.py (MIPS subset scanner) and 6 synthetic tests.
- Found by static reading: the single overlay loader and its wrapper, the
  16-byte overlay header, the scene manager and its 11 scenes, the file number
  of each overlay, unload order and memory-clearing rules. Notes: overlays.md.
- Not run in an emulator; no dispatch code yet.

## 2026-11-22: task 2a review and commit (Claude Code)

- Reviewed the staged task 2a source and notes, reran the 9 metadata tests
  and the staged game-data guard (0 problems), then committed the checkpoint.

## 2026-11-22: shared-folder handoff refresh

- Updated local, ignored AI_HANDOFF.md for Claude Code with the completed task
  2a checkpoint, local evidence, verification commands and next task.
- Released the stale ownership claim and recorded staged-but-uncommitted task
  2a changes. Source implementation preserved; no commit or push in this session.

## 2026-11-22: Phase 1 task 2a, main metadata and analyzer boundary

- Added a narrow, hash-checked preparation tool for the European main ELF. It
  writes a new ignored analysis copy, supplies three stripped section names and
  verifies entry/program headers/loaded bytes remain unchanged [confirmed].
- Recovered all 228 imports / 25 libraries; every identifier matches the separate
  Phase 0 inventory. Nine original synthetic metadata tests pass [confirmed].
- Isolated the scan stall: incomplete final instruction accepted by range check,
  causing a heap bookkeeping overrun. Original five-byte sample and baseline
  analyzer on named main both fail under AddressSanitizer [confirmed].
- Recorded a small MIT analyzer patch with its upstream notice; used a separate
  external worktree. Corrected synthetic probe passes 40 boundary checks, and
  analyzer on named main completes without reported sanitizer errors. Existing
  upstream Debug tests pass 27/27 [confirmed].
- Corrected CLI scan completes in 0.165 s here; 19,445 heuristic candidates,
  154 discovered import-thunk targets vs 228 declared imports. Counts do not prove
  code coverage or guest correctness. All 12 input ELF hashes remain unchanged.
- Next: prove overlay loading/call rules. No full translator/runtime adopted,
  guest execution, console deployment, playable acceptance or release.

## 2026-11-22: Phase 1 task 1, external PSP-to-C tool evaluation

- User authorized Phase 1. Pinned sp00nznet/psprecomp at
  `14749c1f9a94cecf55070d676ce80bc61cf49a5f` outside this repo, unmodified.
  MSVC x64 Debug build succeeded; 27/27 upstream tests passed [confirmed].
- Ran info/cover/funcs/emit on the main ELF and all 11 overlays: 48 zero-exit
  command results. Emitted game C remains ignored under build/evaluation/.
- Compiled 290/296 baseline C units to objects; six main units failed with
  invalid float-literal suffixes. Overlay units all compiled individually;
  no native program was linked or run [confirmed].
- Found missing stripped metadata (zero imports), a zero-function overlay,
  incomplete opcode emission and public-symbol collisions [confirmed].
  A separate metadata-only copy preserved all loaded bytes and exposed all
  228 imports to the existing parser; discovery on it timed out after 90 s.
- Added original synthetic probes reproducing zero-entry omission, correct
  fixed-entry discovery, malformed vector-immediate C and prefix collisions.
  A synthetic compile independently reproduced the main C2059 error.
- Decision: useful external diagnostic candidate, not production-ready unchanged.
  Details and next task: docs/notes/tool-evaluation.md. No outside source/GPL
  adoption, ISO changes, host game boot, console deployment or gameplay claim.

## 2026-11-22: OLED connection and controller check

- User authorized connecting to and controlling the powered-on OLED.
- Used the existing installed Switch Remote server through a local-only bridge;
  its tools were configured but not exposed directly in this chat.
- Remote status and FTP root listing passed. Direct screen captures confirmed
  HOME selection moved one place right and returned to its original tile.
- Released the remote controller with detachController. No app launched, no
  console files changed, no build installed. This is a connection/control test,
  not a DBZ gameplay test. Device details and captures remain ignored locally.

## 2026-11-22: Phase 0 recon and repository safeguards

- Read project instructions/plan first, then reread the user's expanded research
  instructions during the session. Read Riders/Heroes reference notes and selected
  source for method and platform boundaries; adopted no outside code.
- Created a private GitHub repository and installed game-data guard/hooks before
  the first source-only commit. Ignored game files, builds and local handoff notes.
- Copied the user's ISO without changing it; original and copy SHA-1 matched
  [confirmed]. Identified ULES01456 version 1.00 and decrypted the main EBOOT
  locally with an external tool kept outside the repo.
- Main ELF is ET_EXEC at `0x08804040`, with 228 imports in 25 libraries [confirmed].
  Found 11 ELF archive members sharing `0x08A81660` [confirmed]; likely overlays,
  loader/lifetime not proved. Corrected archive size decoding; all 950 declared
  gzip streams decompressed with matching lengths [confirmed].
- Wrote recon, import identifiers and expanded reuse research. Existing US
  TTTRecomp is a specific reference, not a validated port of this European copy.
- Guard: 15 synthetic checks passed, including actual hooks and deleted-history
  rejection. Installed hook passed on staged project files [confirmed].
- No game/runtime implementation, generated game C, boot or gameplay test.
  Phase 1 proposed; stopped for the user's next instruction. Nothing playable
  accepted or released.
