# Phase 0 recon

2026-11-22. Static inspection only. [confirmed] means checked directly;
[likely] is an inference; [uncertain] needs more evidence. No game was booted,
recompiled or tested for play. No third-party source was copied into the project.

## 1. Game identity and safe working copy

- [confirmed] Original: `<home>/Desktop/Dragon Ball Z - Tenkaichi Tag Team/Dragon Ball Z - Tenkaichi Tag Team.iso`.
  Working copy: `<repo>/baserom/game.iso`. The original was only read.
- [confirmed] ISO9660, 2,048-byte sectors, 1,373,667,328 bytes. No CSO conversion needed.
- [confirmed] Original and copy SHA-1: `99b9075c67e17a2f99716f5d0b356e34623bd385`.
  This proves the copy matches; no independent preservation-database match was checked.
- [confirmed] PARAM.SFO: title Dragon Ball Z: Tenkaichi Tag Team; disc ID `ULES01456`;
  disc version `1.00`; minimum system version `6.31`; category `UG`.
- [likely] European release, based on the ULES identifier. Language coverage and
  whether the image is an unmodified retail dump were not independently verified.

## 2. Disc layout

[confirmed] The ISO has 14 files. Important entries:

| Path | Bytes | Finding |
|---|---:|---|
| `PSP_GAME/SYSDIR/EBOOT.BIN` | 2,667,616 | Encrypted PSP wrapper, module DBTVS |
| `PSP_GAME/SYSDIR/BOOT.BIN` | 2,667,272 | Entirely zero; not a usable decrypted executable |
| `PSP_GAME/SYSDIR/OPNSSMP.BIN` | 880 | PSP wrapper; role not established |
| `PSP_GAME/USRDIR/PACKFILE.BIN` | 630,165,504 | Main archive, PacK identifier |
| `PSP_GAME/INSDIR/DATAINST.BIN` | 628,638,544 | Optional-install container; not unpacked |
| `PSP_GAME/SYSDIR/UPDATE/EBOOT.BIN` | 5,540,576 | Firmware updater, not the game |
| `PSP_GAME/SYSDIR/UPDATE/DATA.BIN` | 26,120,416 | Updater data; not analysed |

There are no loose `.prx`, `.at3` or `.pmf` filenames in the ISO. That does not
mean the game has no overlays, compressed sound or movies.

## 3. Main executable

- [confirmed] Encrypted EBOOT SHA-1: `272e18eb11b022067a7f592b2cd9f0f18ee6ae9d`.
- [confirmed] Decrypted locally with the official John-K/pspdecrypt 1.0 executable.
  The external tool and its archive are in a temporary folder outside the repo.
  No tool source or key table was imported. Tool release archive SHA-256:
  `b915e4ce30f4e0309c71b7044584a466be17a181f889f40bcc7f6fd82ba2d045`.
- [confirmed] Decrypted output: `<repo>/baserom/recon/EBOOT.elf`, 2,667,272 bytes;
  SHA-1 `d1d930da28e48dd4ba49d6659035d62c803a1edd`.
- [confirmed] ELF32, little-endian MIPS, type `ET_EXEC` (2), not a relocatable
  main PRX. Entry `0x08804040`; module `DBTVS`, version `0x0101`;
  global pointer `0x08AF7DB0`.
- [confirmed] Two load segments: `0x08804040`, 2,610,717 file/memory bytes;
  `0x08AEFD30`, 132 file bytes / 486,620 memory bytes. Zero-fill matters.
- [confirmed] Executable-marked main section: 2,499,184 bytes. 23 sections;
  section names and ordinary symbol tables are absent. A fixed main address does
  not remove the need to handle separately loaded executable overlays.

## 4. Archive and executable overlays

- [confirmed] PacK header declares 14,097 index slots. Table begins at `0x10`,
  with 16 bytes per slot. Payload area starts at `0x38000`.
- [confirmed] For slot i: the offset word XOR `0x4B636150`, multiplied by `0x800`,
  gives the offset relative to the payload area. The size word XOR i gives the
  unpacked length. The packed-size word XOR `0x4B636150` gives the gzip length;
  zero denotes an uncompressed member. These are format constants, not PSP keys.
- [confirmed] All 950 declared gzip streams decompressed and matched their
  corrected unpacked lengths. One initial interpretation omitted size XOR i;
  the length checks caught it. All inspected nonempty member ranges fit the archive.
  Empty/sentinel slot semantics remain [uncertain].
- [confirmed] Eleven ELF32 MIPS members have executable-marked segments, type
  `0xFFA8`, entry field zero, and the same segment address `0x08A81660`:

| Archive slot | Archive offset | ELF bytes |
|---:|---:|---:|
| 6 | `0x00898800` | 9,344 |
| 10 | `0x008A5800` | 472 |
| 12 | `0x008A6800` | 59,468 |
| 16 | `0x00931800` | 13,792 |
| 20 | `0x009A3800` | 66,824 |
| 23 | `0x00A81000` | 10,784 |
| 31 | `0x017C0800` | 158,324 |
| 47 | `0x030DB000` | 68,540 |
| 55 | `0x04038800` | 246,968 |
| 64 | `0x04E0E800` | 55,996 |
| 13932 | `0x1DA51800` | 450,096 |

- [likely] These are game-managed overlays sharing one memory window. They have
  no standard module-name identifiers identified in this inspection. Do not call
  them ordinary system PRXs: standard PSP PRX type is `0xFFA0`, not `0xFFA8`.
- [confirmed, 2b] Entry convention, loader, lifetime and use of every member are
  now in [overlays.md](overlays.md). Earlier note: their entry convention, loader,
  lifetime, external-call binding and whether every member is used were unknown.
  Key static-recompilation risk: the same guest address may identify different
  code after an overlay swap. Dispatch needs overlay identity as well as address.
- System-module loading is also present in the import table. Which firmware
  modules are loaded at run time remains [uncertain]; updater files are not a
  source for the native runtime.

## 5. System imports, graphics, vector unit and media

[confirmed] The main module imports 228 functions from 25 libraries, and no
variables. Every NID is recorded in [imports.md](imports.md). Counts by library:

| Libraries | Function imports |
|---|---|
| ThreadManForUser / SysMemUserForUser / UtilsForUser | 31 / 6 / 7 |
| IoFileMgrForUser / Kernel_Library / StdioForUser | 9 / 9 / 3 |
| ModuleMgrForUser / LoadExecForUser / sceSuspendForUser | 5 / 2 / 3 |
| sceGe_user / sceDisplay / sceCtrl | 12 / 6 / 3 |
| sceSasCore / sceAudio / sceAtrac3plus / sceMpeg | 27 / 12 / 5 / 23 |
| sceUtility / scePower / sceImpose / sceUmdUser | 21 / 4 / 2 / 5 |
| sceNet / sceNetAdhoc / sceNetAdhocctl | 2 / 6 / 12 |
| sceNetAdhocMatching / sceWlanDrv | 11 / 2 |

- [confirmed] GE imports include list enqueue `0xAB49E76A`, enqueue-head
  `0x1C0D95A6`, stall update `0xE0D68148`, list sync `0x03444EB4`, draw sync
  `0xB287BD61`, and callback registration `0xA4FC06A4`. [likely] A command-list
  interpreter with stall/sync/callback behaviour is the right graphics boundary.
  Actual list contents, draw formats and timing have not been captured.
- [likely] VFPU is substantial: a static scan of executable-marked main sections
  found many vector load/store and COP2 instruction classes, plus prefix-class
  candidates: 93 source, 59 target, 297 destination prefixes. This is not a traced
  instruction-coverage count: embedded data may be counted and other VFPU classes
  were not fully decoded. Prefixes and register aliasing need tests from the start.
- [confirmed] ATRAC imports include low-level init `0x1575D64B`, low-level decode
  `0x0C116E1B`, reinit `0x132F1ECA`, allocate `0x780F88D1`, release `0x61EB33F5`.
  [likely] Headerless ATRAC buffers are important; ordinary RIFF/AT3 filename
  searches are insufficient. Exact ATRAC subtype and stream parameters are [uncertain].
- [confirmed] SAS mixing and MPEG APIs are imported. [likely] SAS handles voice /
  effects, ATRAC handles compressed sound, MPEG handles movies. [uncertain] Actual
  VAG/ADPCM variants, movie container layout, codecs and stream counts. No validated
  standalone PSMF or RIFF member was identified by this bounded inspection.
- [confirmed] Ad-hoc networking APIs are imported. No network runtime or online
  work has started. Import presence is not proof that a call executes.

## 6. Existing tools and licences

Checked upstream material on 2026-11-22. These are candidates, not validated
dependencies. Licence labels are upstream statements, not a completed source audit.
The updated project rules permit credited MIT/BSD/zlib reuse. GPL source adoption
must await the owner's one-time GPL decision. Other licences need checking before
reuse. No outside code was adopted during this session. The expanded survey and
reuse decisions are in [research.md](research.md).

| Project / primary source | Licence | Possible use and limit |
|---|---|---|
| [sp00nznet/psprecomp](https://github.com/sp00nznet/psprecomp) | MIT | C11 Allegrex-to-C, ELF/PRX analysis, partial VFPU/HLE/GE. Upstream alpha; its README has inconsistent rendering-status text. Best first C-toolkit candidate to evaluate, not proof DBZ works. |
| [jessicanataliagta/PSPRecomp](https://github.com/jessicanataliagta/PSPRecomp) | MIT framework; profile dependencies differ | ELF/PRX framework, analyzer and C++ emitter, VCS profile. It emits C++, so differs from our C goal; no bundled EBOOT decryption. |
| [Tzeentchnet/psp-recomp](https://github.com/Tzeentchnet/psp-recomp) | GPL-2.0-or-later, plus component licences | MIPS-to-C/runtime with Killzone and Metal Gear Acid integrations. Upstream explicitly adopts PPSSPP media code and has interpreter fallback. Study only unless owner approves reuse and its scope. |
| [ghidra-allegrex](https://github.com/kotcrab/ghidra-allegrex) | Apache-2.0 | ELF/PRX loading, relocations, Allegrex disassembly. VFPU decompiler does not model prefixes fully. No Ghidra analysis run here. |
| [pspdev/prxtool](https://github.com/pspdev/prxtool/blob/master/LICENSE) | Academic Free License 2.0 | PRX metadata, NID and relocation inspection. Not run; must check compatibility with this game's nonstandard overlays. |
| [PSPSDK](https://github.com/pspdev/pspsdk/blob/master/LICENSE) | BSD-style; check individual components | API definitions and ABI reference, not a ready-made desktop runtime. |
| [John-K/pspdecrypt](https://github.com/John-K/pspdecrypt) | GPL-3.0 | Used as a separate local tool only. Never vendor its embedded key tables. |
| [PPSSPP](https://github.com/hrydgard/ppsspp) | GPL-2.0-or-later; component exceptions | Behaviour/reference testing, debugger and GE/VFPU semantics. Reading is allowed; copying requires owner approval. Not booted this session. |
| [Editor_indice_Py](https://github.com/kastomd/Editor_indice_Py) | Apache-2.0 upstream | Game-specific archive/audio research; describes headerless AT3 and PPHD/VAG tooling. Not installed or copied; bundled dependencies need separate review. |

Upstream revisions observed: sp00nznet `14749c1f9a94cecf55070d676ce80bc61cf49a5f`;
Jessica `f6e7d415c7f447b934cc3865a31eb725f353d659`;
Tzeentchnet `5aea9d3c8fdbb37697748113d0616850dd2cb5db`.

Archive format reference read, not copied: [author's tenkaichi_pack.bms](https://aluigi.altervista.org/bms/tenkaichi_pack.bms).
Its generic layout helped inspection; this copy also requires the measured size-XOR-slot rule.
NID names checked against PPSSPP's [GE table](https://github.com/hrydgard/ppsspp/blob/master/Core/HLE/sceGe.cpp)
and [ATRAC table](https://github.com/hrydgard/ppsspp/blob/master/Core/HLE/sceAtrac.cpp).
PSP ELF type reference: [PSPElfTypes.h](https://github.com/hrydgard/ppsspp/blob/master/Core/ELF/PSPElfTypes.h).

## 7. What carries over from Riders and Heroes

Read Riders' CLAUDE.md, handoff, architecture note, guard, recomp-tool layout and
Switch platform entry; Heroes' recon and initial history. No source copied.
Borrow their source-only setup, local output, status/evidence words, PC-first
tests, scripted inputs/captures, and careful module-lifetime handling. Later,
review the Switch platform interface for paths, input, audio and renderer hosting.
The PowerPC translator, GX renderer, GameCube devices and module assumptions do
not implement Allegrex, GE, VFPU or PSP firmware. Even Riders' early OS-HLE plan
changed during implementation; copy the method, not stale architecture decisions.

## 8. Limits and next step

Phase 0 delivered a safe copy, executable/overlay/import inventory, archive checks
and tool research. No port implementation or native boot exists. Tests cover the
copy hashes, parsed structures, all 950 declared gzip members and repository guard;
they do not validate gameplay, firmware module loading, rendering or media decoding.
Next: the proposed Phase 1 in [plan.md](plan.md). Stop here until the owner starts it.
