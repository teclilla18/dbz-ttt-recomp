# Vendored psprecomp

- Source: https://github.com/sp00nznet/psprecomp (MIT, see LICENSE here).
- Revision: 14749c1f9a94cecf55070d676ce80bc61cf49a5f (v0.6.0).
- Removed from this copy: `.github/`, the key template `docs/psp_keys.example.txt`,
  and all decryption code (`keys.c/h`, `crypto/aes`, `crypto/cmac`, `crypto/kirk`,
  their tests, `docs/DECRYPT.md`, and the `decrypt`/`kirk1` commands), and
  `docs/BRINGUP.md` (it quotes another game's disassembly). Two files were
  re-saved as UTF-8. This
  repository keeps no decryption code or keys (AGENTS.md rule 1).

## Our changes (DBZ TTT), all original code

Recompiler (`tools/allegrexrecomp`):
- Analyzer reads only complete 4-byte words (`tools/patches/psprecomp-complete-word.patch`).
- `[[function]]` hints: extra function starts fed into discovery.
- Whole-number VFPU immediates emitted as `(float)N` (was the invalid `Nf`).
- `vcst` uses its five-bit constant selector (bits 20..16), instead of the
  unrelated source-register field. Original synthetic generated-C execution
  test covers 19 defined constants in all four vector widths.
- `bvf/bvt/bvfl/bvtl` test their selected VFPU condition bit (previously always
  false). Original compiled synthetic branch programs cover 3,072 cases,
  including likely-slot nullification and condition changes in delay slots.
- New ops: `vcmovt/vcmovf`, VFPU random (`vrnds/vrndi/vrndf1/vrndf2`),
  `floor.w.s`, `ceil.w.s`, `round.w.s`.

Runtime (`src/`):
- GE relative addressing: ORIGIN/OFFSET_ADDR supply an additive offset separate
  from BASE, with 28-bit targets for vertices, indices, JUMP and CALL. CALL/RET
  saves/restores that offset; return stacks survive list stalls. List/stall
  pointers collapse cached aliases to the same 28-bit addresses. Original
  synthetic display-list pixel tests in test_ge_address.c.
- `psp_dispatch_clear_range` and label re-registration, for overlays that share
  one address window.
- Threads: per-thread `$k0` block, start arguments copied onto the new stack,
  message boxes, lightweight mutexes, `sceKernelExtendThreadStack`, aliases.
- Files: raw disc device (`umd0:`/`umd1:`, sector units), `sce_lbn..._size...`
  disc windows, start sector in `sceIoGetstat`.
- `src/hle/extra.c`: about 110 more firmware entry points (memory, display, disc,
  dialogs and save data, model query, movie/music/network placeholders).
- Optional call trace (`PSPRECOMP_TRACE_HLE=N`), scripted buttons
  (`psp_ctrl_script`), sceSasCore and sceAudioOutput2 additions.
