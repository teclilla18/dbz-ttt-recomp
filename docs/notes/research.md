# Reuse proven tools before building PSP

## PC stage-load investigation (2026-11-22)

- Rechecked [psprecomp's threading contract](https://github.com/sp00nznet/psprecomp/blob/main/docs/THREADING.md)
  (MIT): scheduling is cooperative; a guest loop without a wait can starve the
  other threads. Reuse its existing function-entry and back-edge tracing to
  distinguish that case from repeated firmware work. No new upstream code copied.
- Rechecked [PSPSDK interrupt documentation](https://pspdev.github.io/pspsdk/group__IntrMan.html)
  ([BSD-style licence](https://github.com/pspdev/pspsdk/blob/master/LICENSE)):
  suspend returns the prior interrupt state for resume. Frequent suspend/resume
  calls alone do not identify an interrupt wait. Interface facts only.
- [Independent PSPRecomp decoder](https://github.com/jessicanataliagta/PSPRecomp/blob/main/src/decoder.cpp)
  (MIT) and [VFPU instruction documentation](https://davidgfnet.github.io/psp-vfpu-docs/#vcst.s)
  ([GPL-3.0 documentation/tests](https://github.com/davidgfnet/psp-vfpu-docs/blob/master/LICENSE)):
  `vcst` selects a constant with a five-bit immediate, not its source-register
  field. Learned instruction facts; copied no code, pseudocode or tests.
  Our emitter correction uses bits 20..16; our synthetic test generates,
  compiles and executes all four widths with mathematical expected values.
- The same VFPU documentation describes `bvf`, `bvt`, `bvfl` and `bvtl` as
  tests of a selected VFPU condition bit. Our emitter previously fell through
  to a constant-false condition for all four. Original synthetic programs now
  exercise six condition bits, all 64 combinations, true/false branches and
  ordinary/likely delay slots, including a slot that changes the condition.
  No reference implementation or test code copied.

## Original survey

PC rendering follow-up, 2026-11-22: reviewed
[PSPSDK GU documentation](https://pspdev.github.io/pspsdk/group__GU.html)
and [GE matrix definitions](https://github.com/pspdev/pspsdk/blob/master/src/ge/pspge.h)
(BSD-style PSPSDK licence, already credited below and in README). Learned the
public bone-matrix/weight-format interfaces: skinning accepts up to eight
matrices/weights. Our runtime explicitly rejects weighted vertex formats;
whether that explains the observed black exploration field still needs a GE
trace. No PSPSDK source adopted and no renderer change made in this follow-up.

Survey checked 2026-11-22. **Evaluate an existing PSP C toolchain before writing
a new translator.** Released Zelda and Sonic ports establish the method, while
an existing Tenkaichi Tag Team project directly supports investigating overlays.
Our local European copy still needs its own loading rules and tests: eleven
executable members share one address window. The strongest reusable ideas are
module-aware dispatch, independent CPU checks, small host interfaces and local
own-copy setup. No external source was adopted during Phase 0. Tools described
as promising are evaluation candidates, not proof that this game works.

**Phase 1 follow-up:** [the pinned C11 tool was evaluated locally](tool-evaluation.md).
It passed its own 27 tests and emitted C for all twelve inputs, but missed the
stripped main metadata and exposed compilation, discovery and overlay-isolation
gaps. Keep it as an external diagnostic candidate; production selection remains
open. No upstream source was imported and no game code was executed.

**Metadata follow-up:** [task 2a](metadata.md) now preserves the main's loaded
bytes while supplying stripped section names, verifies all 228 imports, and
fixes the analyzer's incomplete-word range check. A small MIT-derived patch is
recorded with the upstream notice; toolkit binaries/source trees remain external.
Original synthetic probes plus a memory checker reproduce the baseline overrun
and verify the correction. PSPSDK's module-info definition supplied layout facts;
no PSPSDK source was copied. Production translator selection remains open.

Evidence: [confirmed] means directly inspected local material or linked upstream
source/documentation, as specified. Upstream playability claims were not tested
by us. [likely] identifies an inference; [uncertain] identifies an open point.
Local findings are in [recon.md](recon.md), including the exact ISO identity and
[228 system imports](imports.md). No game was booted in this session.

## Released ports prove a method, not PSP compatibility

The compiler and the playable game project have different responsibilities and
often different licences. Producing valid host source does not implement the
original machine's graphics, operating system or timing. That distinction should
remain visible in every milestone here.

| Project and inspected source | Licence | What works or teaches us | Proposed use and limit |
|---|---|---|---|
| [N64Recomp](https://github.com/N64Recomp/N64Recomp/blob/main/README.md), [call resolution](https://github.com/N64Recomp/N64Recomp/blob/main/src/recompilation.cpp), [C emitter](https://github.com/N64Recomp/N64Recomp/blob/main/src/cgenerator.cpp) | [MIT](https://github.com/N64Recomp/N64Recomp/blob/main/LICENSE) tool; dependencies separate | MIPS-to-C analysis/emission handles delay slots, indirect targets and overlays. Its source resolves function candidates using section identity as well as address. | **Idea; eligible code candidate later.** This is concrete precedent for DBZ's shared-address overlays. It targets N64 MIPS, not Allegrex/VFPU, and needs a runtime. |
| [Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp), [v1.2.2 release](https://github.com/Zelda64Recomp/Zelda64Recomp/releases/tag/v1.2.2), [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime) | GPL-3.0 game/runtime, separate dependencies | A released Majora's Mask port demonstrates the compiler-plus-runtime approach beyond a tool demo. | **Idea only now.** Borrow input validation and explicit runtime boundaries. N64Recomp's MIT licence does not cover this whole stack. No independent play test here. |
| [XenonRecomp](https://github.com/hedge-dev/XenonRecomp/blob/main/README.md), [analysis/configuration source](https://github.com/hedge-dev/XenonRecomp/blob/main/XenonRecomp/recompiler.cpp) | [MIT](https://github.com/hedge-dev/XenonRecomp/blob/main/LICENSE.md) tool | PowerPC-to-C++ pipeline separates analysis, configuration and emission. Its guidance covers function boundaries, jump-table overrides and correctness before optimization. | **Idea.** Record uncertain targets and controlled overrides. Its PowerPC decoder, endian rules and vector operations do not implement PSP. It explicitly does not supply the game runtime. |
| [UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp/blob/main/README.md), [v1.0.3 release](https://github.com/hedge-dev/UnleashedRecomp/releases/tag/v1.0.3), [XenosRecomp](https://github.com/hedge-dev/XenosRecomp) | GPL-3.0 game project; components separate | Released Sonic Unleashed PC port uses XenonRecomp plus shader translation. Setup checks the player's input files. | **Idea only now.** Borrow compatible-input checks and reproducible local setup. Xbox CPU/shader translation is not a PSP GE frontend. |
| [ran-j/PS2Recomp](https://github.com/ran-j/PS2Recomp/blob/main/README.md), [control-flow emitter](https://github.com/ran-j/PS2Recomp/blob/main/ps2xRecomp/src/lib/control_flow_emitter.cpp) | [GPL-3.0](https://github.com/ran-j/PS2Recomp/blob/main/LICENSE) | Experimental R5900-to-C++ analysis/runtime. Source distinguishes branch-likely delay slots and indirect targets; docs acknowledge partial hardware paths and VU/GS limits. | **Idea only now.** Borrow explicit unsupported-operation reporting and cold-boot checkpoints. No complete playable title was verified in this survey. PS2 MMI/VU is not PSP VFPU. |

[likely] **Overlay identity belongs in dispatch from the start.** N64Recomp's
section-aware resolution addresses the same class of ambiguity that our DBZ
inventory exposes. It does not establish DBZ's entry convention or loader
lifetime. Those require direct analysis of this copy before choosing an address
or relocation strategy.

## PSP tools offer a starting point with important gaps

Several unrelated projects use similar PSPRecomp names. Evaluate their exact
repository and revision, not the name alone. This session inspected selected
source and docs; none of these compilers was run against our ISO.

| Project and inspected source | Licence | Useful part | Proposed use and limit |
|---|---|---|---|
| [sp00nznet/psprecomp](https://github.com/sp00nznet/psprecomp/blob/main/README.md), [analyzer](https://github.com/sp00nznet/psprecomp/blob/main/tools/allegrexrecomp/analyze.c), [emitter](https://github.com/sp00nznet/psprecomp/blob/main/tools/allegrexrecomp/emit.c) | [MIT](https://github.com/sp00nznet/psprecomp/blob/main/LICENSE) | C11 Allegrex-to-C, ELF/PRX inspection, partial VFPU/HLE/GE. Source separates imports, instruction ownership, delay slots and interior dispatch labels. | **Adopted 2026-11-22 as the translator/runtime base (copied, decryption code removed).** Output matches our C goal. Alpha status, experimental threading and inconsistent rendering prose require caution; [changelog](https://github.com/sp00nznet/psprecomp/blob/main/CHANGELOG.md) narrows the claimed playable microgames. Never vendor its decryption/key files. |
| [jessicanataliagta/PSPRecomp](https://github.com/jessicanataliagta/PSPRecomp), [ELF loader](https://github.com/jessicanataliagta/PSPRecomp/blob/main/src/elf32.cpp), [decoder](https://github.com/jessicanataliagta/PSPRecomp/blob/main/src/decoder.cpp) | MIT framework; profile dependencies separate | Generic framework and title profiles; loader range checks; decoder classifies branch delay slots, vector loads/stores and prefixes. | **Comparison tool/ideas.** Emits C++20 rather than our requested C. No EBOOT decryption bundled. Classification is not proof of complete VFPU semantics; retain [provenance boundaries](https://github.com/jessicanataliagta/PSPRecomp/blob/main/docs/SOURCE_PROVENANCE.md). |
| [Tzeentchnet/psp-recomp](https://github.com/Tzeentchnet/psp-recomp/blob/main/README.md), [porting workflow](https://github.com/Tzeentchnet/psp-recomp/blob/main/docs/PORTING_WORKFLOW.md) | GPL-2.0-or-later combined project; ATRAC component LGPL-2.1-or-later | MIPS-to-C tooling/runtime used by Killzone and Metal Gear Acid projects; overlay/boot-contract procedures, graphics and media integration. | **Ideas only now.** Upstream explicitly adopts PPSSPP MPEG code and an FFmpeg-derived decoder. Interpreter fallback must not conceal missing static coverage. Runtime reuse awaits the GPL decision. |
| [wizardengineer/psprecomp](https://github.com/wizardengineer/psprecomp) | GPL-2.0 upstream label | Separate Rust analysis/emission pipeline with C++17 and SDL2/OpenGL runtime. | **Secondary idea reference.** No DBZ compatibility or full component audit established; not the first C-output candidate. |

The C toolkit's [oracle guide](https://github.com/sp00nznet/psprecomp/blob/main/docs/ORACLE.md)
provides a useful testing rule: compare against an independent PPSSPP process
and hardware-backed PSP tests. Two execution paths sharing the same arithmetic
helpers can share the same bug. [likely] Own synthetic samples should exercise
delay slots, bit-preserving transfers, FPU corner cases and VFPU prefix/register
aliasing before performance work.

Our main is [confirmed locally] fixed-address `ET_EXEC`; its archive members use
type `0xFFA8`. A standard PRX feature claim does not prove support for that layout.
Observed upstream revisions are recorded in [recon.md](recon.md); pin the actual
revision again when adopting a tool.

## Emulator knowledge should guide measured firmware boundaries

**PPSSPP is the main behaviour reference; PSPSDK documents interfaces; JPCSP is
a useful second implementation.** They do not remove the need to test this game.

| Reference | Licence | What we learned and can use |
|---|---|---|
| [PPSSPP thread implementation](https://github.com/hrydgard/ppsspp/blob/master/Core/HLE/sceKernelThread.cpp), [GE table/state](https://github.com/hrydgard/ppsspp/blob/master/Core/HLE/sceGe.cpp), [ATRAC table](https://github.com/hrydgard/ppsspp/blob/master/Core/HLE/sceAtrac.cpp) | GPL-2.0-or-later in inspected files | **Behaviour ideas and NID facts now.** Separate waits for audio, GE, I/O, threads and networking matter. GE enqueue/stall/sync/callback imports are a state machine. Low-level ATRAC imports support investigating raw buffers rather than assuming RIFF files. Code reuse awaits GPL approval. |
| [PPSSPP developer tools](https://www.ppsspp.org/docs/development/developer-tools/) | Emulator GPL-2.0-or-later; component notices differ | **External tool later.** Logging, debugger/GPU diagnostics and executable dumps support controlled reference runs. Dumps and captures remain local. No reference run occurred here. |
| [JPCSP GE](https://github.com/jpcsp/jpcsp/blob/master/src/jpcsp/HLE/modules/sceGe_user.java), [VFPU state](https://github.com/jpcsp/jpcsp/blob/master/src/jpcsp/Allegrex/VfpuState.java) | [GPL-3.0-or-later](https://github.com/jpcsp/jpcsp/blob/master/COPYING) | **Ideas only.** Queues and callbacks cross-check GE explanations. Raw integer bits coexist with float values so vector transfers preserve bit patterns. Test NaNs, overlapping registers and prefix lifetime; do not assume either emulator is a hardware proof. |
| [PSPSDK docs](https://pspdev.github.io/pspsdk/), [GE header](https://github.com/pspdev/pspsdk/blob/master/src/ge/pspge.h), [NID stubs](https://github.com/pspdev/pspsdk/blob/master/src/ge/sceGe_user.S), [module metadata](https://github.com/pspdev/pspsdk/blob/master/src/user/pspmoduleinfo.h) | [BSD-style three-clause root licence](https://github.com/pspdev/pspsdk/blob/master/LICENSE); check individual files | **Documentation and eligible definitions later.** API/ABI, NID names and metadata help define our firmware boundary. Firmware-call stubs do not implement a desktop OS. |
| [ghidra-allegrex](https://github.com/kotcrab/ghidra-allegrex/blob/master/README.md) | [Apache-2.0](https://github.com/kotcrab/ghidra-allegrex/blob/master/LICENSE) | **External analysis tool.** Allegrex ELF/PRX, relocations and symbol interchange help inspect control flow. Its decompiler explicitly lacks full VFPU prefix semantics. Not run here. |
| [pspdev/prxtool](https://github.com/pspdev/prxtool) | [Academic Free License 2.0](https://github.com/pspdev/prxtool/blob/master/LICENSE) | **External cross-check tool.** PRX metadata, NID names and relocation/disassembly exports; compatibility with nonstandard overlays remains untested. Keep outputs local. |
| [John-K/pspdecrypt](https://github.com/John-K/pspdecrypt/blob/master/Readme.md), [official releases](https://github.com/John-K/pspdecrypt/releases) | GPL-3.0 | **External tool used in Phase 0.** Official 1.0 decrypted the copied main executable. Tool and embedded keys stay outside this repository. No decryption source was adopted; no updater extraction is needed. |

Licence summaries describe inspected notices, not every bundled component.
Under current AGENTS.md, credited MIT/BSD/zlib reuse is allowed; GPL code adoption
waits for the owner's one-time decision. MPL, Apache, ISC and AFL components need
their own notices/obligations checked. No licence decision is needed merely to
complete this survey.

## Existing TTT work and Switch ports sharpen the plan

**A same-game project is a valuable reference, with a different supported dump.**
[MarquitoK/TTTRecomp](https://github.com/MarquitoK/TTTRecomp) is GPL-3.0, C++17,
Windows/OpenGL and explicitly an early alpha for USA `ULUS10537`. Its authored
[ISO build pipeline](https://github.com/MarquitoK/TTTRecomp/blob/main/tools/build_from_iso.py)
lists eleven overlay indices, exactly our local zero-based slots plus one. This
corroborates the overlay investigation, not region equivalence. It expects an
already-ELF EBOOT and USA identity; our `ULES01456` has an encrypted retail wrapper.
Borrow setup and overlay ideas now. Do not adopt its code before GPL approval or
copy its region addresses. Its feature/playability claims were not tested here.

[Editor_indice_Py](https://github.com/kastomd/Editor_indice_Py) declares Apache-2.0
and [documents](https://github.com/kastomd/Editor_indice_Py/blob/master/README.md)
PACKFILE/PCK/PPHD resources, headerless AT3 and VAG conversion. Use those as format
hypotheses or compare an external tool later; exact audio/movie parameters remain
unconfirmed in our copy. Optional codec programs have separate provenance.
[Aluigi's archive script](https://aluigi.altervista.org/bms/tenkaichi_pack.bms)
provided PacK/gzip format facts during recon; no explicit script licence was
established, so no code was copied. Local length checks established the additional
size-XOR-slot rule. [Nimbus](https://github.com/thebabyhippo303/nimbus-recompilation-project)
claims TTT support but keeps relevant source closed: skip as a code dependency.

For eventual online work, PPSSPP's [ad-hoc overview](https://www.ppsspp.org/docs/development/adhoc/overview/)
separates discovery/control, matching and packet transport. Its built-in
[AdhocServer](https://www.ppsspp.org/docs/development/adhoc/adhoc-server/) coordinates
peers on TCP 27312; it does not relay game packets. The separately integrated
postoffice design combines discovery and relay. Current [quickstart](https://www.ppsspp.org/docs/multiplayer/quickstart/)
and [connection guide](https://www.ppsspp.org/docs/multiplayer/how-to-play/) describe
1.20-family relay support and direct/LAN alternatives. These are GPL-project
behaviour/setup references, not deterministic input lockstep. No controlled
TTT-native compatibility test was established; promise neither crossplay nor
successful NAT traversal before testing. Online implementation remains Phase 4.

Local [Riders](https://github.com/teclilla18/sonic-riders-recomp) and
[Heroes](https://github.com/teclilla18/sonic-heroes-recomp) source/notes were read.
Riders' root licence is MPL-2.0. Its [Switch hosting](https://github.com/teclilla18/sonic-riders-recomp/tree/main/runtime/src/platform/switch)
separates SD paths, HID, PCM output and HOME/sleep handling; its
[host input](https://github.com/teclilla18/sonic-riders-recomp/blob/main/runtime/src/platform/host/input_sdl.c)
and local capture code support unattended tests. Borrow those interfaces and
methods. Heroes teaches proving module lifetime before placement; its side-by-side
REL bases are not automatically safe for DBZ's shared window. Their PowerPC/GX
code does not implement Allegrex/GE/VFPU. Any later MPL source reuse must preserve
its obligations and per-file provenance.

| Switch source | Licence | Useful later boundary |
|---|---|---|
| [libnx](https://github.com/switchbrew/libnx), [pad API](https://github.com/switchbrew/libnx/blob/master/nx/include/switch/runtime/pad.h) | [ISC](https://github.com/switchbrew/libnx/blob/master/LICENSE.md) | Homebrew OS/input services. Map PSP controls deliberately; no inherited GameCube masks. |
| [deko3d](https://github.com/devkitPro/deko3d), [Primer](https://github.com/devkitPro/deko3d/blob/master/Primer.md) | [zlib](https://github.com/devkitPro/deko3d/blob/master/LICENSE) | C-facing explicit GPU backend, resources and fences. Targets Switch Tegra X1; does not decode PSP GE commands. |
| [uam](https://github.com/devkitPro/uam/blob/master/LICENSE) | zlib own files; Mesa components retain notices | PC build-time shader tool. Do not presume a console runtime compiler. deko3d docs identify devkitPro's switch-dev packages; record versions when used. |
| [SwitchWakerHD](https://github.com/centollOS/SwitchWakerHD/blob/main/README.md), [installation](https://github.com/centollOS/SwitchWakerHD/blob/main/INSTALL.md), [credits](https://github.com/centollOS/SwitchWakerHD/blob/main/THIRD_PARTY.md) | MPL-2.0 framework; dependencies separate | Source-only local preparation and own-copy validation. PowerPC/Cafe OS/deko3d implementation is a method reference, not a PSP runtime. |
| [Shipwright-Switch](https://github.com/timschneeb/Shipwright-Switch/blob/develop/README.md), [upstream overview](https://github.com/HarbourMasters/Shipwright#project-overview) | Whole-tree reusable licence not established here | Idea reference for PC asset preparation, SD layout and title-mode launching. Decompilation-based/libultraship port; no source adoption. |

No Switch build or hardware test was performed. ARM64 commonality does not prove
native Switch 2 support; its platform and GPU require separate evidence after
Switch 1 works.

## Use these findings one tested step at a time

- **Use:** external pspdecrypt already used; PSPSDK documentation/NID facts;
  evaluate the MIT C11 PSP analyzer/emitter first, with Ghidra/prxtool as needed.
  Keep tools containing key material outside the repository and all game output local.
- **Borrow ideas from:** N64Recomp's section-aware dispatch; released ports'
  input checks; PPSSPP/JPCSP behaviour; TTTRecomp overlay setup; modders' format
  knowledge; Riders/Heroes captures and loader investigation; Switch hosting.
- **Skip now:** GPL source adoption, closed/proprietary dependencies, unknown-opcode
  success stubs, foreign CPU/GPU implementations, USA-only mappings, online work
  and any assumption of automatic Switch 2 compatibility.

The resulting Phase 1 proposal is:

1. Pin and evaluate an existing C-oriented tool; review its exact components.
2. Prove the main/overlay loading and dispatch rules for this European copy.
3. Test Allegrex, FPU and VFPU semantics using our own small samples and independent results.
4. Translate the main and overlays locally with explicit unsupported-target reports.
5. Compile and check that output, then document gaps before Phase 2 runtime work.

These steps match [plan.md](plan.md). Phase 0 stops here. Translation checks,
native boot, user-accepted gameplay and release remain separate milestones.
