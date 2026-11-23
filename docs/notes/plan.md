# Plan (2026-11-22; Phase 1 tool evaluation checkpoint)

Phase 0 is complete. Findings are in [recon.md](recon.md), the full
reuse survey in [research.md](research.md), and system NIDs in [imports.md](imports.md).
Phase 1 task 1 is complete: [external tool evaluation](tool-evaluation.md).
Local trial C was emitted and object compilation checked; nothing was booted.
Production translator selection remains open. Later phases remain proposals.

## Phase 0: recon (first session, no code)

1. Copy the user's game file into `baserom/` and record its SHA-1. If it is a CSO, convert it to ISO first
   (keep the original untouched).
2. Find the executable: `PSP_GAME/SYSDIR/EBOOT.BIN`. Retail EBOOTs are usually encrypted; the decrypted copy must
   be made on the user's machine at setup time and never committed, and no keys go in the repo.
3. List the modules (PRX files the game loads), the imported system functions (by NID), the audio and video
   formats (AT3, PMF movies?), and how it uses the GPU (GE display lists) and the vector unit (VFPU).
4. **Research online** (a real survey, several searches, read the projects' code and docs) and write
   `docs/notes/research.md`: for each project, link, licence, what it does well, and what we can take (tool, code or
   idea). At least:
   - Static recompilers that shipped playable games: N64Recomp (Zelda MM), XenonRecomp / UnleashedRecomp (Xbox 360),
     ps2recomp and similar, any PSP or MIPS recompiler, and the user's own Riders/Heroes ports.
   - PSP knowledge: PPSSPP (OS functions, GE graphics, VFPU, ad-hoc online and its AdhocServer), JPCSP, pspdev /
     PSPSDK documentation, PRX/EBOOT tools (decryption on the player's machine, NID lists).
   - Switch side: what the Riders/Heroes ports already solved, plus other Switch homebrew ports.
   - Anything about this game: modding community tools, file formats, known online setups.
   End with a short "use / borrow ideas from / skip" list and the plan it leads to.
5. Write `docs/notes/recon.md` and show the user the plan for phase 1. Stop there.

## Phase 1: recompiler

Started at the owner's request. Work one step at a time, test and commit each.

1. **Choose the base.** Evaluate the C-oriented PSP tools from the research survey
   on our executable and overlay formats. Study the existing same-game TTTRecomp
   design, accounting for its US version, C++ output and GPL licence. Pin a
   revision and record licences for any selected tool.
   Prefer using an existing tool before writing its replacement. Keep tools with
   embedded decryption keys outside the repo. No GPL code adoption until the
   owner's GPL decision.
   **Checkpoint:** pinned C11 candidate builds and passes 27 upstream tests, but
   misses stripped metadata, stalls after diagnostic names are restored, emits
   unsupported operations/invalid C and does not isolate overlay symbols. Keep
   it external as a diagnostic candidate; do not accept it unchanged.
2. **Prove the loading rules.** Check the fixed main ELF and find how the game
   swaps its 11 executable overlays at `0x08A81660`. Record entry points, memory
   clearing and binding rules. Ordinary relocatable PRXs are a separate case;
   our main ELF is ET_EXEC and the archive overlays use type `0xFFA8`.
   **Next small task:** make metadata/entry discovery reliable, isolate the
   named-copy analysis stall, and verify 228 imports before integrating overlays.
3. **Test CPU translation on our own small samples.** Cover branches and delay
   slots, indirect calls, PSP calling conventions, integer/FPU edge cases, and
   VFPU prefixes/register aliasing. Compare with independent reference results.
4. **Translate the main executable and overlays locally.** Keep generated C,
   diagnostics and game-derived maps in ignored `build/`. Give each overlay its
   own code identity so a shared guest address dispatches to the active overlay.
   Report reachable unsupported instructions or unknown jump targets explicitly.
5. **Check the result before runtime work.** Compile the local output, verify
   loader bounds/zero-fill/import accounting and the synthetic CPU tests, and
   document remaining gaps. This proves translation checks, not a working game.

Phase 2 then handles PC boot, firmware calls, graphics, sound and controls.

## Phase 2: runtime on PC (the fast test loop)

- The PSP's operating system functions (threads, files, memory, timers, audio, controls, save data) reimplemented
  in C, like the Riders runtime's HLE.
- The GE graphics command lists drawn with OpenGL/Vulkan on PC.
- Scripted input and screenshots from day one (Riders' SR_INPUT / SR_SCREENSHOT idea) so tests run unattended.
- Goal: boots to the title screen, then a battle that plays to the end.

## Phase 3: Switch, then Switch 2

- Reuse the Riders/Heroes Switch platform layer (deko3d renderer approach, input, audio, file paths, the setup tool
  that builds on the player's PC). Switch 2 native after Switch 1 runs well.
- 480x272 original resolution: higher internal resolution as an option.

## Phase 4: online play (only after it runs stable)

- The game's multiplayer is ad-hoc (local wireless). Two ways, to decide later with the user: carry the ad-hoc
  packets over the internet (as PPSSPP does, which might even allow play with PPSSPP users), or a lockstep design
  like the Riders Online Beta. Deterministic emulation first, as in Riders.

## Decisions and open points

- [confirmed] User supplied an ISO; identity `ULES01456`, version `1.00`.
- [confirmed] User authorized a private GitHub repo; it must stay private.
- [confirmed] Reading public reference source is allowed. Credited MIT/BSD/zlib
  reuse is allowed by updated AGENTS.md. No outside code has been adopted yet.
- [uncertain] Project licence / approval to adopt GPL source: defer until a
  concrete reuse decision needs it. No GPL adoption is needed for Phase 0.
- [uncertain] Overlay loader behaviour, media subtypes and dynamic firmware
  module list need more analysis; see the limits in recon.md.
