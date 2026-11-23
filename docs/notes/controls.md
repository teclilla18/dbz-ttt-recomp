# OLED controller requirement

Requested by the user on 2026-11-22. **Recorded requirement; not implemented
or tested on Switch.** The current PC Xbox mapping remains the testing baseline.

## Menus

Use the Switch controller's natural labels:

- **A selects/confirms** (send PSP Cross).
- **B goes back/cancels** (send PSP Circle).

Apply this to game menus, including pause and results screens. Other menu
actions keep their defined mappings; no extra X/Y menu behaviour was requested.

## Fighting

In actual combat, in every game mode, use the same physical button positions
as the PC Xbox layout:

| Switch button | Position | PSP action |
|---|---|---|
| B | Bottom (Xbox A position) | Cross |
| A | Right (Xbox B position) | Circle |
| Y | Left (Xbox X position) | Square |
| X | Top (Xbox Y position) | Triangle |

Switch between menu and combat handling automatically. This requirement is
for the OLED's Switch controller, including its normal face-button labels.
Do not change the PC Xbox layout to satisfy it.

## Implementation and validation still needed

1. Identify a confirmed game state for combat versus menu navigation. Overlay
   4 also contains story screens, so its presence alone is insufficient.
2. Map physical Switch input at the platform boundary, before delivering PSP
   button bits. Keep the layout state available safely to the input reader.
3. Avoid carrying a held confirm/cancel button into a different action when
   changing context; verify transitions, pauses, retries and battle results.
4. On OLED, test A-confirm/B-cancel in menus and the four positional battle
   buttons in each mode that starts combat. Keep implemented, tested and
   user-accepted status separate.
