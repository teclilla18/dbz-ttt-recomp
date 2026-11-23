# Phase 1, task 1: evaluate the C-based PSP tool

2026-11-22. **Decision: keep this tool as an external diagnostic candidate;
do not accept it as the production translator yet.** It builds and can generate
C from our files, but direct tests expose missing metadata handling, incomplete
discovery/translation and code that does not compile. No game code was executed.
No runtime, foreign source or decryption keys were adopted into this repository.

Follow-up: [task 2a](metadata.md) adds repeatable section-name preparation and a
credited MIT boundary correction. It resolves the main metadata/scan-stall gap;
the baseline findings below remain the record of the unmodified tool evaluation.

## Tool and scope

- [confirmed] Evaluated [sp00nznet/psprecomp](https://github.com/sp00nznet/psprecomp)
  at commit `14749c1f9a94cecf55070d676ce80bc61cf49a5f`, detached and unmodified.
  The root licence is [MIT](https://github.com/sp00nznet/psprecomp/blob/14749c1f9a94cecf55070d676ce80bc61cf49a5f/LICENSE).
- External checkout/build: `<temp>/dbz-ttt-tool-evaluation/`. Nothing from that
  checkout was vendored, linked into this project or pushed. Only local ELF
  analysis commands were used; no decryption command or key file was needed.
- Host build: Visual Studio 2022, MSVC 14.44.35207, x64 **Debug**, C11.
  Debug keeps the upstream test assertions enabled. Default inline-thread mode;
  experimental cooperative host-thread mode was not evaluated.
- Inputs: existing `baserom/recon/EBOOT.elf` and all eleven existing
  `baserom/pack_recon/overlay_*.elf` files. Original ISO never opened for writing.
  Input paths, outputs, C/object files and raw diagnostics stay ignored locally.
- Destination remains native Switch OLED and Switch 2. Host compiler checks
  below do not establish Switch build compatibility, performance or playability.

## What passed

| Check | Observed result |
|---|---|
| Build unmodified upstream tool/runtime and tests | Passed |
| Run upstream tests with Debug assertions | 27/27 passed, 4.28 seconds |
| `info`, `cover`, `funcs`, `emit` on each of 12 inputs | All 48 invocations exited zero |
| Compile generated baseline C into objects with MSVC | 290/296 passed; six main-file units failed |
| Compile generated overlay C into objects | 108/108 passed individually; not linked or executed |
| Original synthetic probes | Four observations reproduced; details below |

All 48 command successes mean the commands completed, not that their answers
were adequate. Upstream tests cover their own synthetic cases; they are not
independent evidence for our game's CPU/vector behaviour. The probe suite also
records limitations, so its success must not be called translator acceptance.

## What failed or remains incomplete

### 1. Stripped names hide metadata

[confirmed] On the original main ELF the tool reports zero firmware imports,
although our earlier structural inspection found 228. Its parser also uses the
whole first segment as its code extent: 2,610,717 bytes instead of the main
executable section's 2,499,184 bytes. Function discovery reports 11,877 candidates,
58.48% of that segment reached, 229 scattered candidates and 5,752 without a
normal return. These are heuristic outputs, not validated function boundaries
or a measured percentage of the game successfully translated.

The upstream [ELF parser](https://github.com/sp00nznet/psprecomp/blob/14749c1f9a94cecf55070d676ce80bc61cf49a5f/tools/allegrexrecomp/container.c)
locates text, import stubs and module information by section names. This game's
names are stripped. [confirmed] A separate diagnostic ELF under `build/` restored
only the names `.text`, `.sceStub.text` and `.rodata.sceModuleInfo`, using section
facts already recorded during recon. All load-segment bytes and the entry address
were checked byte-identical to the input. The original ELF was not changed.

A small original host program calling the existing parser/import APIs then found
the 52-byte module info, 1,824-byte stub range and **all 228 imports** in that
diagnostic copy. This demonstrates a metadata-recognition gap without adopting
an alternate loader. However, `funcs` on the named copy exceeded a 90-second
timeout and was terminated. Emission for the named copy was therefore not run.
The cause of the analysis stall is [uncertain]; this is not a working fix.

### 2. Accepting the overlay ELF type does not prove overlay support

[confirmed] All eleven type `0xFFA8` ELFs were accepted by the CLI. Their header
entry fields are zero. The tiny 472-byte file produced **zero functions**, yet
both discovery and emission exited successfully. The other ten emitted some
functions, which a call-target scan can find without a valid entry seed. Their
reported reached fractions vary and must not be mistaken for complete coverage.

The tool's [discovery setup](https://github.com/sp00nznet/psprecomp/blob/14749c1f9a94cecf55070d676ce80bc61cf49a5f/tools/allegrexrecomp/main.c)
seeds the header entry and scans direct-call targets. Real overlay entries and
lifetimes remain unknown. Do not invent an entry or relocate this fixed-address
game just to make a report look complete.

### 3. Generated C has a reproducible floating-point literal bug

[confirmed] Six main C units fail to compile, with 18 MSVC `C2059` diagnostics
for invalid number suffixes. A finite whole-number value emitted for the VFPU
immediate instruction can become an integer spelling with `f` appended. That
spelling is invalid ISO C. The upstream
[emitter](https://github.com/sp00nznet/psprecomp/blob/14749c1f9a94cecf55070d676ce80bc61cf49a5f/tools/allegrexrecomp/emit.c)
uses a general floating-point format followed by `f` for finite VFPU immediates.

An original tiny synthetic ELF loading vector constant 1.0 reproduces the same
MSVC `C2059` error. No game bytes are needed for the reproducer. We did not patch
the external emitter or silently repair generated game C during this task.

### 4. Some discovered operations still become explicit failures

[confirmed] Baseline main output contains 239 `vcmov` failure sites, 328 unnamed
VFPU failure sites and 33 `floor.w.s` failure sites, plus 23 `break` sites.
The first three categories need investigation; `break` may be an intentional
guest exception rather than a missing implementation. These are static emitted
site counts, not proof that gameplay reaches each site. Some discovery may have
walked into data. Other VFPU operations already emit helpers, so the CLI's broad
VFPU-trap wording is not a complete account of vector translation.

### 5. Separate output prefixes do not separate overlay symbols

[confirmed] Across the twelve baseline outputs, 2,229 public C symbol names
occur in more than one module; a name can occur in all twelve. Supplying a
different CLI prefix changes output filenames but leaves address-based public
function names shared. An original two-emission synthetic probe reproduces this.

The runtime [dispatch table](https://github.com/sp00nznet/psprecomp/blob/14749c1f9a94cecf55070d676ce80bc61cf49a5f/src/dispatch.c)
also keys registrations by address and replaces a registration at the same
address. Registering all overlays together is therefore not an overlay solution.
We did not link all modules into a native program. A future design must identify
the active overlay, handle direct as well as indirect calls, and clear stale
entries when a differently sized overlay replaces another.

## Reproduce the checkpoint locally

Use an external checkout and build directory, never a submodule for this task.
The following is the tested build configuration; use installed native Windows
CMake/CTest rather than accidentally selecting MSYS variants:

```sh
git -C <external> checkout --detach 14749c1f9a94cecf55070d676ce80bc61cf49a5f
cmake -S <external> -B <external-build> -G "Visual Studio 17 2022" -A x64
cmake --build <external-build> --config Debug --parallel 4
ctest --test-dir <external-build> -C Debug --output-on-failure --timeout 45 -j 4
python tools/probe_psp_tool.py --tool <external-build>/tools/allegrexrecomp/Debug/allegrexrecomp.exe
```

The probe writes only into a **new** subfolder of ignored `build/`; pass
`--output build/evaluation/synthetic-next` when rerunning. It creates original
synthetic ELF samples, runs the external CLI, and checks four observations:
zero-entry discovery omits the function, a fixed entry finds it, whole-number
vector-immediate C is malformed, and different prefixes collide. It exits zero
when those known behaviours are reproduced. This is not an end-to-end CPU test.

The game-input matrix used each command below on the main and eleven overlays,
without `--base`, hints, input patching or emitter changes:

```sh
allegrexrecomp info <local-elf>
allegrexrecomp cover <local-elf>
allegrexrecomp funcs <local-elf>
allegrexrecomp emit <local-elf> <ignored-module-output> <module>
```

Object checks compiled every emitted `.c` separately with MSVC `/std:c11 /c /Od`
and the external tool's `include/` directory. Raw diagnostics must remain local
because compiler errors can include game-derived source lines.

Evidence folder: `build/evaluation/`. It contains configure/build/test logs,
`cli-results.json`, `compile-results.json`, `compile-logs/`, generated C and objects,
`metadata-results.json`, the named diagnostic copy and synthetic probe evidence.
These are local evidence, not distributable project sources.

## Recommendation and next task

**Use the existing analyzer/emitter as a diagnostic tool for now. Keep production
base selection open.** The MIT licence and C output fit our direction, and the
working components justify focused experiments before writing replacements.
The measured gaps prevent selecting this pinned revision unchanged.

Next small task: establish reliable metadata/entry inputs for this game's main
and overlays, isolate the named-copy discovery stall with original synthetic
cases, and verify the 228-import count without scanning data as code. Prove the
game's overlay loading before designing its dispatch integration. The float
literal, missing operations and module naming remain separate acceptance gates.
If those experiments show a poor fit, compare the next researched tool before
committing to a translator rewrite. No GPL adoption is needed for this checkpoint.

No host game boot, Switch build, console install, gameplay, GPU/audio correctness,
or performance claim follows from these results. Nothing playable is accepted
or released.
