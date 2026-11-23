/* DBZ TTT overlays: one 0xFFA8 overlay per scene, all at one window.
 * Facts in docs/notes/overlays.md. The table is generated per player by
 * tools/build_game.py into build/gen/overlay_table.c. */
#ifndef DBZ_OVERLAYS_H
#define DBZ_OVERLAYS_H

#include <stdint.h>

#define DBZ_OVERLAY_COUNT  11
#define DBZ_WINDOW_LO      0x08A81660u
#define DBZ_WINDOW_HI      0x08AEFD30u

typedef struct {
    uint32_t label;            /* header word 0, 1..11 */
    uint32_t base;             /* always DBZ_WINDOW_LO */
    uint32_t memsz;
    void (*register_fns)(void);
} dbz_overlay;

extern const dbz_overlay dbz_overlays[DBZ_OVERLAY_COUNT];

/* Called by the hooked guest runners (see tools/build_game.py). */
void dbz_overlay_loaded(void);
void dbz_overlay_unloaded(void);
uint32_t dbz_overlay_current(void);   /* label, or 0 when none */

#endif
