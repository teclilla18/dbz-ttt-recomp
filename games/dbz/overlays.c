/* Overlay swap for the recompiled game.
 *
 * The guest loader still copies the overlay bytes into guest memory (its data
 * and vtables live there). Just before the guest runs the overlay's start-up
 * list we point the dispatch table at that overlay's recompiled functions;
 * just after its clean-up list we forget them. A call into the window with
 * no overlay loaded is then a named dispatch miss, never a stale function. */
#include "dbz_overlays.h"

#include <psprecomp/dispatch.h>
#include <psprecomp/mem.h>

#include <stdio.h>
#include <stdlib.h>

static uint32_t g_current;

uint32_t dbz_overlay_current(void) { return g_current; }

void dbz_overlay_loaded(void) {
    const uint32_t label = psp_read32(DBZ_WINDOW_LO);
    psp_dispatch_clear_range(DBZ_WINDOW_LO, DBZ_WINDOW_HI);
    g_current = 0;
    for (int i = 0; i < DBZ_OVERLAY_COUNT; i++) {
        if (dbz_overlays[i].label == label) {
            dbz_overlays[i].register_fns();
            g_current = label;
            if (getenv("DBZ_LOG_OVERLAYS"))
                fprintf(stderr, "[dbz] overlay %u loaded\n", (unsigned)label);
            return;
        }
    }
    fprintf(stderr, "[dbz] unknown overlay label %u at 0x%08X\n", (unsigned)label, DBZ_WINDOW_LO);
    abort();
}

void dbz_overlay_unloaded(void) {
    if (getenv("DBZ_LOG_OVERLAYS"))
        fprintf(stderr, "[dbz] overlay %u unloaded\n", (unsigned)g_current);
    psp_dispatch_clear_range(DBZ_WINDOW_LO, DBZ_WINDOW_HI);
    g_current = 0;
}
