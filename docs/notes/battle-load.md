# First-stage PC loading stall

Investigated 2026-11-22, Windows x64, MSVC Release. Game-derived source,
screenshots, debugger maps and raw traces remain under ignored `build/`.

## Evidence

- [confirmed] The original scripted New Game / Dragon Walker / first-stage
  route stalls on the "Now Loading" screen after overlay 4 loads.
- [confirmed] With entry/back-edge tracing enabled, watchdogs at 160, 200 and
  240 seconds all show frame 8515 and loop `0x08AA54FC`. The back-edge count
  grows from about 31.7 to 84.4 billion while the function-entry count stays
  at 74,164,416. This is guest computation without further firmware calls.
  These are diagnostic observations, not frame-rate measurements.
- [confirmed] A read-only register snapshot of the stalled process shows
  `f0` and `f15` are NaN, with angle bounds of approximately minus/plus pi.
  The loop cannot finish its angle comparison with that input.
- [confirmed] The preceding math helper uses `vcst` selectors 3 and 8,
  but the emitter passes the generic source-register field (zero in those
  instructions). The selector actually occupies bits 20..16. The runtime
  constant table already handles the intended indices.
- [confirmed] Correcting `vcst` alone still stalls at frame 8518 in the same
  loop (160/200/240-second watchdogs). It fixes a real error but is insufficient.
- [confirmed] All four VFPU branch conditions are absent from the emitter's
  branch-condition switch. Each becomes constant false. Local math probes
  show ordinary finite inputs taking wrong paths and zero-axis inputs producing
  NaN. The runtime already supplies the condition bits; the emitter ignores them.

## Correction and regression check

The emitter now extracts the five-bit `vcst` selector from its instruction
and emits the selected condition-bit test for all four VFPU branch forms.
No game function is replaced and no loop is skipped. See
[research sources](research.md#pc-stage-load-investigation-2026-11-22).

An original synthetic fixture encodes all 32 selector values at all four
widths, runs the production analyzer/emitter, compiles its C, then executes
the 19 defined nonzero constants. Expected values use mathematical definitions,
and the check also verifies destination lanes and untouched registers.

- [confirmed] Before the correction, that execution test fails: constants
  become zero. After the correction it passes.
- A second original fixture translates and executes 3,072 branch cases: all
  six meaningful condition bits, all 64 bit combinations, four branch forms
  and two delay-slot variants. One counts executed slots; the other changes
  the condition in the slot to check sampling before execution.
- [confirmed] Before the branch correction, 1,536 of 3,072 cases fail; after
  it, all pass. The likely forms also preserve an unexecuted slot's state.
- [confirmed] Local probes invoking the game's angle helper with synthetic
  inputs now return approximately pi/2 for (1, 0), pi/4 for (1, 1), and zero
  for (0, 0). With only the constant correction those last two kinds of
  calculation were wrong or non-finite. Probe sources/output stay ignored;
  the committed regression fixtures contain only original synthetic code.
- [confirmed] Toolkit tests: 27/27 pass in MSVC Release with real threads.
  Existing project Python tests: 17/17 pass. Regeneration and PC linking pass.

## Reproduction

[confirmed] With both corrections, the same fresh-save route passes the
loading screen and reaches story pages by frame 9000. Added Cross presses at
9500, 10100 and 10700 advance those pages; the capture at 11400 shows later
story dialogue. This validates the original stall correction, not playable
combat or battle completion.

The final watchdogs progress from frame 17790 at 160 seconds to 25797 at
200 seconds; the run was then stopped deliberately. The original runs stayed
at 8515/8518. These are virtual vblank counts, not measured rendering FPS.

1. Configure PC with `-DDBZ_TRACE=ON`, regenerate with `tools/build_game.py`
   after rebuilding the translator, then build target `dbz_ttt`.
2. Use a fresh, separate filesystem root: its `disc/` may be a directory
   junction to the extracted disc; its `ms/` must be separate from existing saves.
   Pass this root as the executable's second argument, after the main ELF.
3. Set `DBZ_DUMP_SEC=160,200,240`, `DBZ_LOG_OVERLAYS=1`, and a screenshot folder
   with `DBZ_SHOTS`. The watchdog accepts increasing nominal elapsed times in
   seconds; writing a dump adds a little delay to later snapshots.
4. Use the established scripted route:
   `700:cross,2400:start,2800:up,3000:cross,3600:cross,4300:left,4500:cross,5100:cross,5700:cross,6300:cross,6900:cross,7500:cross,8000:left,8200:cross`.
5. Compare the frames, loop address, entry totals and captures after stage start.
   Watchdog snapshots are best-effort diagnostics, not synchronized CPU dumps.
   Disable `DBZ_TRACE` for normal play; it also enables stack diagnostics.

## Further PC check before handoff (2026-11-22)

- [confirmed] The rebuilt normal PC Release (`DBZ_TRACE=OFF`) advances through
  story, "Start Mission: Find Gohan" (capture 14400) and the movement tutorial
  (16200). At 23100 the exploration HUD is visible over a black field, with no
  visible world or character. This is not a successful gameplay validation.
  Cause remains unproven. Original loading-loop fix still holds on this route.
- Evidence: ignored `build/battle-play/route1/`, including `press.txt`, screenshots
  and logs. Separate saves; original/default saves untouched. The 180-second
  watchdog reached frame 21470. Run stopped deliberately after these captures.
- Next probe: choose Free Battle directly to separate battle loading/rendering
  from campaign exploration. Initial route in `build/battle-play/free1/press.txt`:
  use the boot/name/save presses through 5100, then Down at 5500 and 5700,
  Cross at 6000, and Cross every 600 frames from 6600 through 27000.
  [confirmed] Main Menu selection and overlay 5 load; deliberately stopped for
  handoff at the owner's request before combat. Last capture is frame 6000.
- Runtime source currently rejects weighted vertex formats in `vertex_layout`
  (`ge.c`). That is a plausible contributor to missing 3D, not a proven cause
  in these scenes. Before implementing skinning, collect GE statistics and a
  targeted `PSPRECOMP_TRACE_GE=<frame>` log to establish the actual draw formats.
  `psp_ge_dump_stats()` exists but is not called by the current watchdog.
- No renderer/input source changed during these checks. The next agent should
  claim this investigation and show a narrow plan before implementation.
