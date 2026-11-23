# Missing 3D: GE relative addresses

Investigated 2026-11-22 on Windows x64, MSVC Release, `DBZ_TRACE=OFF`.
Status: implemented and tested for relative addressing. Rendering still has
separate gaps; gameplay is not accepted or released.

## Diagnosis

- [confirmed] The fresh-save Free Battle route reaches character selection,
  stage selection and an active fight. Combat HUD/timer and health changes are
  visible, but the scene is severely corrupted. Not playable validation.
- [confirmed] In the trace at vblanks 12000–12005, the game issues ORIGIN and
  then small vertex offsets. Example fact: the origin at `0x096AC2E0` plus
  offset `0xD4` names `0x096AC3B4`. The old interpreter uses `0xD4` instead.
  It stores the origin but never applies it to vertex or list addresses.
- [confirmed] OFFSET_ADDR also overwrites BASE in the old interpreter rather
  than maintaining a separate additive offset. Its temporary return stack
  disappears at each stall and does not save/restore the offset.
- [confirmed] Traced combat formats include `0x00011A` (16-bit texture and
  position, 4444 colour) and `0x00019F` (float texture/position, 8888 colour),
  alongside 2D HUD formats. These contain no weights. Skinning support was
  therefore not selected as this correction.
- [confirmed] A separate CPU gap is exercised: `vsocp` at `0x0880F3E8` reaches
  the unsupported-instruction report. Its visual/gameplay effect is unproven;
  it is unchanged in this task.

## Correction

The interpreter resolves addresses from the BASE high field, the low address
field and the separate additive offset, then masks to 28 bits. ORIGIN takes its
own command address; OFFSET_ADDR supplies the offset directly. Vertices,
indices, JUMP and CALL share the same resolution. CALL saves the return address
and offset; RET restores both. Queue-owned stacks preserve them while stalled.
The existing eight-entry stack limit remains an approximation, not a hardware
depth validation. Bounding-box jumps remain conservatively untaken.

List entry/stall pointers are normalized to the same 28-bit form as jump
targets. Without this, a jump from an uncached list alias can pass its stop
point and read commands the CPU has not finished writing. An intermediate
run exposed this at frame 781; that incomplete change was not committed.

Sources and licence boundaries: [research.md](research.md#missing-3d-diagnosis-2026-11-22).
No emulator code or game functions copied.

## Verification

- Original `test_ge_address.c`: eleven synthetic list cases assert the exact
  coloured rectangle and its untouched border. Covers origin-relative vertices
  and indices; BASE/offset update order; carry and address wrap; relative JUMP;
  nested CALL/RET; a stall inside the nested call; initial/updated uncached
  alias stop points; and BASE changes surviving a return.
- [confirmed] Before the relative-address correction: seven of the initial
  eight cases fail, including bad reads near address zero. The additional
  alias fixture separately reproduces execution beyond a stop point before
  address normalization. After the complete correction all eleven cases pass.
- [confirmed] Toolkit: 28/28 tests pass with MSVC Release and real threads.
  PC target links successfully. No retranslation needed for this runtime fix.
- One full-suite run failed an existing HLE sleep-duration assertion once;
  its isolated recheck and the subsequent full 28-test run passed. No thread
  timing implementation changed. This remains an intermittent test observation.
- Graphics statistics now appear in the host's timed/frame dumps. The existing
  `PSPRECOMP_TRACE_GE` accepts `first,last` inclusive, since an exact frame can
  miss lists submitted on alternate vblanks. Dumps remain best-effort snapshots.
- [confirmed] Complete correction, fresh-save Free Battle: arena terrain/sky
  appear at frame 12000; later capture 14700 is black apart from the HUD.
  At 26400 the run reaches a WIN results screen with arena visible behind it.
  The simulation progresses, but fighter visibility and playable combat remain
  unverified. Run deliberately stopped after saving these captures.
- [confirmed] In the corrected combat trace, uploaded world-matrix elements
  are non-finite and many projected coordinates become invalid. The separate
  unsupported `vsocp` is still exercised. A local own-input probe of the guest
  math helper at `0x0880F398` returns NaN for input 1 when scratch vector lanes
  start at 2, versus pi/2 when they start at zero. That shows an additional
  state-dependent math failure; it is not corrected by this GE change.

## Local evidence and reproduction

All derived captures, raw GE traces and logs stay under ignored `build/`.
Do not upload them.

- `build/ge-diag/free-base/`: initial Free Battle diagnostic, exact-frame trace
  missed submissions. Capture 13200 shows combat corruption; timer/health
  continue. `free-trace/`: original runtime, range trace 12000–12005 and
  `DBZ_DUMP_AT=12500`; stopped deliberately after collecting it.
- `build/ge-diag/free-fixed/`: intermediate address-only correction, stopped
  after the cached-alias stall. `free-fixed2/`: complete correction including
  alias normalization, same script and fresh root.
- Input script: `build/battle-play/free1/press.txt` (boot/save, Free Battle,
  then Cross every 600 virtual frames). Each run has a separate `ms/`; `disc/`
  is a junction to the extracted disc. Default/user saves remain untouched.
- `DBZ_SHOTS=<existing folder>`, `DBZ_LOG_OVERLAYS=1`,
  `DBZ_DUMP_SEC=150,210,270`, `DBZ_DUMP_AT=12500`,
  `PSPRECOMP_TRACE_GE=12000,12001`. Virtual vblank counts are not FPS.
