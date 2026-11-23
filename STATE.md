# Current state

Updated 2026-11-22. Phase 0 complete; Phase 1 metadata checkpoint complete.
Destination: native Switch OLED and Switch 2. PC is for development/checks.

Task 2a reviewed and committed by Claude Code (9 tests and the staged guard
rerun before commit). Next: the overlay-loader task below, plan first.

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

Identify the game's actual overlay loader and call/entry convention. Do not
invent entry points from zero header entries. Prove lifetime and memory-clearing
rules before choosing dispatch integration. Work one tested commit at a time.

## Where to read

- [Main metadata and analyzer correction](docs/notes/metadata.md)
- [Plan](docs/notes/plan.md)
- [Prior tool evaluation](docs/notes/tool-evaluation.md)
- [Recon](docs/notes/recon.md)
- [Research](docs/notes/research.md)
- [Imports](docs/notes/imports.md)
