# Overlay loading rules (Phase 1 task 2b)

Found 2026-11-22 by static reading of the European main ELF (ULES01456 v1.00)
with the original scanner `tools/mips_scan.py`. Nothing was executed; no
emulator run. Addresses are guest (PSP) addresses. Tags as in AGENTS.md.

## The window

- [confirmed] Main's first load segment ends at `0x08A8165D`; its bss segment
  starts at `0x08AEFD30`. The gap `0x08A81660`–`0x08AEFD30` (452,304 bytes) is
  the overlay window. Every overlay's single segment starts at `0x08A81660`.
- [confirmed] The largest overlay (archive file 13932) fills the window exactly
  (memory size `0x6E6D0`). All others are smaller.
- [confirmed] Main code never forms `0x08A81660` itself. The loader takes the
  address from the overlay's own program header.

## The loader: `0x08A572F4` load_overlay(path, size_out)

[confirmed] steps, in order:
1. Open the file (sceIoOpen, read-only). Read the 52-byte ELF header.
2. Check magic `7F 'E' 'L' 'F'`, type `0xFFA8`, program-header size 32, count 1.
   Any mismatch: close and return 0.
3. Seek to and read the one program header; seek to its file offset and read
   file-size bytes straight to its virtual address.
4. If memory size > file size, zero the tail (`0x08A50F58`, memset-like).
5. Close. Optionally store the byte count. Invalidate the instruction cache over
   the first min(file size, 16 KB), then write back/invalidate the whole data cache.
6. Return the load address (= `0x08A81660`).

- [confirmed] Only bytes up to the overlay's memory size are written. The rest
  of the window keeps whatever the previous overlay left (no full clear).
- [confirmed] No relocations are applied; overlays are fixed-address code.

## Wrapper: `0x089C35B4` load_overlay_by_file(file_number)

- [confirmed] File number 0 → return 0. Otherwise build a path from the number
  (`0x08830314`, using a runtime table at `0x08B5F280`; path form [uncertain]),
  wait for the UMD drive to be ready, call the loader, and on failure wait one
  second and retry forever. Then run the overlay's start-up functions (`0x08A57500`).
- [confirmed] `0x08A572F4` has exactly one caller (this wrapper), and the
  wrapper is called only from the 11 scene load functions. There are no pointer
  references to either one in main data or in any overlay.

## Overlay header (first 16 bytes at `0x08A81660`)

| Offset | Size | Meaning |
|---:|---:|---|
| 0 | 4 | Label number 1–11 (each once). Not the scene number. [confirmed] |
| 4 | 2 | Start-up function count |
| 6 | 2 | Clean-up record count |
| 8 | 4 | Address of start-up function pointer list |
| 12 | 4 | Address of clean-up record pointer list |

- [confirmed] Start-up (`0x08A57500`): calls each listed function with no
  arguments, right after loading. Clean-up (`0x08A57578`): for each record
  {arg1, arg0, function}, calls function(arg0, arg1). This looks like C++
  static constructors/destructors [likely].
- [confirmed] Code begins at `0x08A81670` (header + 16). The ELF entry field is
  zero because main never "starts" an overlay. It calls overlay functions directly.

## Scenes: which overlay, when

The scene manager (`0x089C34AC`, runs each update) compares the requested scene
with the current one. If they differ, it calls the current scene's unload
function, then switches through a jump table at `0x08A705E0` to the new scene's
load function (`0x089C3360`). [confirmed]

A table at `0x08A7EC20` has 11 entries of {file number (s16), pad, base (u32)}.
Code only reads the file numbers. Each scene load stores the base; unload clears it.

| Scene | Archive file | Archive offset | Header label | Mem size |
|---:|---:|---:|---:|---:|
| 1 | 6 | 0x00898800 | 1 | 0x2634 |
| 2 | 10 | 0x008A5800 | 2 | 0xF8 |
| 3 | 12 | 0x008A6800 | 3 | 0x14938 |
| 4 | 16 | 0x00931800 | 11 | 0x34F4 |
| 5 | 13932 | 0x1DA51800 | 4 | 0x6E6D0 |
| 6 | 23 | 0x00A81000 | 5 | 0x2940 |
| 7 | 31 | 0x017C0800 | 6 | 0x26990 |
| 8 | 55 | 0x04038800 | 7 | 0x3C454 |
| 9 | 64 | 0x04E0E800 | 8 | 0xDAA4 |
| 10 | 47 | 0x030DB000 | 9 | 0x10AD8 |
| 11 | 20 | 0x009A3800 | 10 | 0x106B8 |

[confirmed] all 11 archive ELF members are used, one per scene. What each scene
shows (title, menu, battle...) is [uncertain] until run.

Scene unload order [confirmed]: delete the scene object (virtual call), run the
overlay's clean-up records (`0x089C368C` → `0x08A57578`), clear the base slot.
The bytes stay in memory until the next load overwrites them.

## Calls between main and overlays

- [confirmed] Main contains direct `jal` calls to about 150 different addresses
  inside the window, from many places (mostly `0x08844xxx`–`0x0884Bxxx` and
  `0x089C4xxx`–`0x089C5xxx`). Main also stores addresses of overlay vtables.
  So some main code only runs while a particular overlay is loaded.
- [likely] Overlays call main functions directly; no overlay calls the loader.

## What this means for the recompiler (design input, not built)

1. Replace the loader natively: still copy the bytes into guest memory (overlays
   hold data and vtables too), zero the tail, and record which overlay is loaded.
   Cache operations become no-ops.
2. A call or jump to a window address must dispatch by (loaded overlay, address),
   not by address alone. The loaded overlay can be read from the header label at
   `0x08A81660` or tracked by the native loader.
3. A main call into the window with no function there in the loaded overlay is
   a bug: stop with a clear message, never guess.
4. Each overlay is translated separately with its own symbol prefix.

## Not done yet

- No run in an emulator to watch a real load ([uncertain] path string form).
- Which main functions run under which scene is not mapped.
- No dispatch code written.
