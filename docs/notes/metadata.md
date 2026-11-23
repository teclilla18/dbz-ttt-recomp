# Main metadata and analyzer boundary correction

2026-11-22. Phase 1 task 2a. **Implemented and tested:** create a validated local
analysis copy of the European main ELF, recover its import inventory and avoid
the external analyzer's incomplete-instruction buffer overrun. No guest code
was run. Overlay loading, complete translation and a native runtime remain open.

## The repeatable preparation step

`tools/prepare_main_elf.py` accepts only the previously recorded decrypted main
SHA-1 `d1d930da28e48dd4ba49d6659035d62c803a1edd`. This is deliberately a narrow
adapter, not a general ELF repair tool. Unknown regions/versions are refused.

It checks ELF32 little-endian MIPS ET_EXEC headers, segment/file bounds, and the
known layout of these three stripped sections before assigning names:

| Section | Name supplied for analysis | Address | File offset | Bytes |
|---:|---|---:|---:|---:|
| 2 | `.text` | `0x08804040` | `0x1018` | 2,499,184 |
| 7 | `.sceStub.text` | `0x08A664EC` | `0x2634C4` | 1,824 |
| 9 | `.rodata.sceModuleInfo` | `0x08A66C70` | `0x263C48` | 52 |

Section 9 has ALLOC and WRITE flags (3); its analysis name does not change flags.
The adapter adds a section-name table, updates the relevant section-header
fields, and preserves the entire ELF header, all program headers and every
file byte covered by a PT_LOAD segment. It refuses any metadata edit overlapping
loaded bytes or program headers. The entry, addresses, zero-fill sizes and game
instructions stay unchanged. It neither decrypts nor relocates anything.

Import descriptors are independently range-checked using the actual load-segment
mapping: descriptor lengths, library identifiers, NID arrays and thunk extents.
The report contains 228 imports across 25 libraries. Every library name and NID
matches the separate Phase 0 inventory exactly [confirmed]. This is the full
declared table, not a claim that all imported calls execute during gameplay.

Only new output files below ignored `build/` are permitted. Existing outputs
are refused. The ELF and companion JSON report are local game-derived artifacts:
never commit or upload either. The original input is read only.

```sh
python tools/prepare_main_elf.py
python -m unittest discover -s tests -p test_prepare_main_elf.py -v
```

Defaults: `baserom/recon/EBOOT.elf` to `build/metadata/main-analysis.elf` and
`main-analysis.json`. Use `--output build/metadata/another-analysis.elf` for a new
run. The tested output SHA-1 is `05382d576cc6068c5fec0b7567ae861253b71f47`.

## Why the named copy stalled

[confirmed] The pinned analyzer's range check accepted an aligned address if
its first byte lay in the segment, even when fewer than four bytes remained.
Its bookkeeping arrays allocate one entry per **complete** four-byte word.
The main's first load segment has 2,610,717 bytes: one byte remains after its
complete words. A heuristic data-pointer scan could therefore add that partial
word as a code seed, leading to an out-of-bounds bookkeeping write.

An original five-byte synthetic segment reproduced the heap-buffer-overflow
under MSVC AddressSanitizer. An instrumented analyzer reading our local named
copy showed the same sanitizer failure, at the same entry-marking operation.
No game instructions were executed to reproduce it. Instrumented stage counters
also showed discovery was progressing before heap damage; the earlier 90-second
timeout alone did not reveal the cause.

`tools/patches/psprecomp-complete-word.patch` changes the external analyzer's
range predicate to require all four instruction bytes and uses subtraction
instead of overflowing end-address addition. It applies to the pinned revision
`14749c1f9a94cecf55070d676ce80bc61cf49a5f`. The original external checkout remains
clean; a separate external worktree contains only this correction.

The patch includes a small amount of MIT upstream context. Preserve
[the upstream MIT notice](../../third_party/licenses/psprecomp.txt). Original
upstream author: sp00nznet; our boundary change and regression probe were written
here. This is the first recorded MIT-derived patch, not adoption of a full
translator/runtime. No GPL source or key material was imported.

## Evidence and practical limits

| Check | Result |
|---|---|
| Metadata preparation tests, original synthetic fixtures | 9/9 passed |
| Boundary probe against unmodified analyzer | 7/38 checks failed; discovery skipped |
| Boundary probe against corrected analyzer, with AddressSanitizer | 40 checks passed, no reported memory error |
| Original synthetic partial-word reproduction | Baseline heap-buffer-overflow confirmed |
| Analyzer with the actual named local input | Baseline sanitizer failure; corrected analyzer completed with no reported sanitizer error |
| Corrected external tool's existing Debug tests | 27/27 passed |
| Corrected CLI `funcs` on repeatably prepared main | Exit 0 in 0.165 seconds on this host |
| Source ELF checks | All 12 prior evaluation input hashes unchanged |

The scan reports 19,445 heuristic function candidates, 154 distinct directly
discovered import-thunk targets and the full table of 228 imports / 25 libraries.
These counts describe different things: the discovered subset is not a missing
table, and discovery does not prove a call is used in play. The scan also reports
81 invalid decoded words. It still follows heuristics through the wider image;
the tool's percentage is not a verified code-coverage or correctness result.
No final function map, complete entry set or code/data separation is accepted yet.

The vector-immediate C bug, unsupported operations and overlay namespace problem
from [task 1](tool-evaluation.md) were not changed. This task did not regenerate
or compile the main's full C corpus, link a port, run guest code or test hardware.

## External build and probe procedure

Use a separate worktree/build outside the repository, at the pinned revision.
Apply only our tracked patch, then build in x64 Debug as in the evaluation note:

```sh
git -C <external-worktree> apply --check <repo>/tools/patches/psprecomp-complete-word.patch
git -C <external-worktree> apply <repo>/tools/patches/psprecomp-complete-word.patch
cmake -S <external-worktree> -B <external-build> -G "Visual Studio 17 2022" -A x64
cmake --build <external-build> --config Debug --parallel 4
ctest --test-dir <external-build> -C Debug --output-on-failure --timeout 45 -j 4
allegrexrecomp funcs <repo>/build/metadata/main-analysis.elf
```

For the original boundary probe, run MSVC from an initialized native x64 tools
environment. Compile `tests/psp_complete_word.c` with the external analyzer and
decoder directly; include that tool's header directory. Keep objects/PDB/executable
below `build/metadata/`. The tested switches were `/MD /Zi /fsanitize=address`.
This avoids linking differently configured Debug static libraries into the probe.
With the corrected sources, the probe must print 40 checks and zero failures.

Raw evidence stays in `build/metadata/` (preparation report, CLI scan, boundary
results and sanitizer logs), plus `build/evaluation/` (original reproduction,
instrumented-stage diagnostics and upstream regression logs). Full sanitizer
reports may contain machine-specific paths; do not paste them into tracked notes.

## Sources and next task

- [Pinned upstream analyzer](https://github.com/sp00nznet/psprecomp/blob/14749c1f9a94cecf55070d676ce80bc61cf49a5f/tools/allegrexrecomp/analyze.c):
  MIT source, locally patched range check only.
- [Pinned upstream ELF parser](https://github.com/sp00nznet/psprecomp/blob/14749c1f9a94cecf55070d676ce80bc61cf49a5f/tools/allegrexrecomp/container.c):
  MIT API used externally to cross-check metadata; no parser source vendored.
- [PSPSDK module-info definition](https://github.com/pspdev/pspsdk/blob/master/src/user/pspmoduleinfo.h):
  BSD documentation/layout reference; no source copied into our adapter.

Next small task: identify how the game loads and calls its eleven shared-address
overlays. Recover entries from actual loader/call evidence, not from the zero
header entries or arbitrary seeds. Keep the current translator as an external
candidate until its other acceptance gaps are resolved. Switch OLED and Switch 2
remain the final targets; nothing playable is accepted or released.
