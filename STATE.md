# Current state

Updated 2026-11-22. Phase 0 complete; Phase 1 tasks 1, 2a and 2b complete.
Destination: native Switch OLED and Switch 2. PC is for development/checks.

## Latest checkpoint: overlay loading rules (task 2b)

- [confirmed, static reading only] One loader (`0x08A572F4`) with one wrapper;
  16-byte overlay header with start-up/clean-up lists; code at `0x08A81670`.
- [confirmed] 11 scenes, each loads one overlay; all 11 archive ELFs used. The
  scene manager unloads the old scene before loading the next. Only the overlay's
  own memory size is written/zeroed; the rest of the window is left as it was.
- [confirmed] Main calls ~150 addresses inside the window directly, so dispatch
  must use (loaded overlay, address). Details: docs/notes/overlays.md.
- Implemented: original tools/mips_scan.py; tested with 6 synthetic tests
  (15 tests total pass). Not run in an emulator; no dispatch code yet.

## Implemented

- Source-only guard and installed hooks; private repository.
- Recon, NID inventory, reuse survey and plan.
- Hash-checked main metadata adapter: new local analysis copy only; entry,
  program headers and all loaded bytes preserved. Unsupported inputs refused.
- Small MIT-licensed external-analyzer boundary patch, with upstream notice.
  Full source tree/build stays external; no adopted full translator or runtime.
- Original metadata tests and synthetic analyzer boundary/probe tests.

## Tested this checkpoint

- [confirmed] 9 metadata tests pass using original synthetic ELF fixtures.
- [confirmed] European main preparation sees 228 imports in 25 libraries; every
  library/NID matches the independent Phase 0 inventory. Loaded bytes unchanged.
- [confirmed] Five-byte synthetic input and analyzer reading the actual named
  main both expose baseline heap-buffer-overflow under AddressSanitizer.
- [confirmed] Correction requires a complete four-byte instruction and avoids
  overflowing end-address addition. 40 boundary checks pass under sanitizer;
  corrected analyzer reading the named main reports no sanitizer error.
- [confirmed] Corrected external tool's 27 upstream x64 Debug tests pass.
- [confirmed] Prepared main scan finishes in 0.165 s here, exit 0. Reports 19,445
  heuristic function candidates, 154 discovered import-thunk targets, and the full
  declared table of 228 imports. It still has 81 invalid decoded words; these
  are static analysis results, not execution coverage or validated boundaries.
- [confirmed] All 12 input ELF hashes unchanged. Baseline external tree clean;
  separate worktree contains only the tracked analyzer correction.

## Earlier tests and limits

- Phase 1 task 1: baseline tool built; 27 tests passed. 48 CLI commands exited
  zero; 290/296 emitted C units compiled to objects. Six main units failed due
  invalid vector-immediate literals. No complete native game program linked.
- Other unresolved gates: unsupported operations, complete code/data separation,
  correct overlay entries/lifetime, module symbol/dispatch isolation and CPU/FPU/
  VFPU semantics. Production translator choice remains OPEN.
- Original/copy ISO hash matched; ULES01456 v1.00, main entry 0x08804040. Eleven
  archive ELF code pieces share 0x08A81660. All 950 gzip members decoded.
- OLED connection/control checked previously: status, read-only FTP, screenshots,
  reversible HOME navigation. Selection restored; remote controller detached.
- No guest code executed, renderer/audio behaviour validated, Switch build,
  console install, gameplay or online test. Nothing playable accepted/released.

## Source and local material

- MIT patch notice: third_party/licenses/psprecomp.txt. No GPL source adoption
  approved or performed. Credited MIT/BSD/zlib reuse allowed by AGENTS.md.
- External candidate revision: 14749c1f9a94cecf55070d676ce80bc61cf49a5f.
  Baseline/build and corrected worktree/build live under a temporary tools folder.
- Game ELF, generated C, objects and logs stay ignored in baserom/ and build/.
  This checkpoint evidence is in build/metadata/ and build/evaluation/.
- Original ISO untouched; no decryption keys in this repository.

## Next small task

Confirm one real overlay load in an emulator debugger (path string, scene order),
then start plan step 3: CPU translation tests on our own small samples.

## Where to read

- [Overlay loading rules](docs/notes/overlays.md)
- [Main metadata and analyzer correction](docs/notes/metadata.md)
- [Plan](docs/notes/plan.md)
- [Prior tool evaluation](docs/notes/tool-evaluation.md)
- [Recon](docs/notes/recon.md)
- [Research](docs/notes/research.md)
- [Imports](docs/notes/imports.md)
