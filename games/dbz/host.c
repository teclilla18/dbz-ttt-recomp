/* Dragon Ball Z: Tenkaichi Tag Team -- PC host for the recompiled game.
 *
 *   dbz_ttt.exe [main.elf] [fs-root]
 *
 * fs-root holds disc/ (the disc files, from tools/extract_disc.py) and ms/
 * (the memory stick, for saves). Defaults come from the build. */
#include "dbz_overlays.h"

#include <psprecomp/cpu.h>
#include <psprecomp/mem.h>
#include <psprecomp/hle.h>
#include <psprecomp/module.h>
#include <psprecomp/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void psp_recomp_register(void);   /* build/gen/main/dbz_register.c */
void dbz_register_imports(void);  /* build/gen/main/dbz_imports.c (tools/build_game.py) */

/* Main is linked at 0x08804040; keep the module mapping below the heap. */
#define MAIN_SPAN (0x08C00000u - 0x08804040u)

/* psp_module_load copies only the first PT_LOAD; main has a second one
 * (a few initialised bytes at 0x08AEFD30 ahead of .bss). Copy the rest. */
static int load_extra_segments(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    unsigned char h[52];
    if (fread(h, 1, 52, f) != 52) { fclose(f); return -1; }
    uint32_t phoff = h[28] | h[29] << 8 | h[30] << 16 | (uint32_t)h[31] << 24;
    int phnum = h[44] | h[45] << 8;
    for (int i = 1; i < phnum; i++) {
        uint32_t p[8];
        fseek(f, (long)(phoff + 32u * i), SEEK_SET);
        if (fread(p, 4, 8, f) != 8) { fclose(f); return -1; }
        if (p[0] != 1 || !p[4]) continue;
        unsigned char *buf = malloc(p[4]);
        fseek(f, (long)p[1], SEEK_SET);
        if (!buf || fread(buf, 1, p[4], f) != p[4] || psp_mem_write_block(p[2], buf, p[4]) != 0) {
            free(buf); fclose(f); return -1;
        }
        free(buf);
    }
    fclose(f);
    return 0;
}

/* DBZ_DUMP_SEC=<s>: a separate host thread prints thread states and recent
 * firmware calls after that many seconds, even if a guest thread never yields. */
#ifdef _WIN32
#include <windows.h>
static DWORD WINAPI watchdog(LPVOID arg) {
    Sleep((DWORD)(intptr_t)arg * 1000u);
#else
#include <pthread.h>
#include <unistd.h>
static void *watchdog(void *arg) {
    sleep((unsigned)(intptr_t)arg);
#endif
    fprintf(stderr, "[dbz] watchdog dump at frame %llu\n", (unsigned long long)psp_display_vblanks());
    psp_threadman_dump(stderr);
    psp_hle_dump_ring(stderr);
    fflush(stderr);
    return 0;
}

int main(int argc, char **argv) {
    const char *elf = argc > 1 ? argv[1] : DBZ_MAIN_ELF;
    const char *root = argc > 2 ? argv[2] : DBZ_FS_ROOT;

    if (psp_mem_init() != 0) { fprintf(stderr, "memory init failed\n"); return 1; }
    psp_module m;
    if (psp_module_load(elf, MAIN_SPAN, &m) != 0 || load_extra_segments(elf) != 0) {
        fprintf(stderr, "usage: %s [decrypted main .elf] [fs root]\n", argv[0]);
        return 1;
    }
    if (getenv("DBZ_NO_CTORS")) m.ctors = 0, m.nctors = 0;
    psp_cpu_reset();
    psp_hle_init();
    psp_io_set_root(root);
    /* The game reads its archive through the raw disc device (umd1:). */
    {
        const char *iso = getenv("DBZ_ISO") ? getenv("DBZ_ISO") : DBZ_ISO_PATH;
        psp_io_set_iso(iso);
    }
    psp_recomp_register();
    dbz_register_imports();

    if (psp_window_start("Dragon Ball Z: Tenkaichi Tag Team (recompiled)", 0) != 0) {
        fprintf(stderr, "could not open a window\n");
        return 1;
    }
    printf("DBZ TTT -- main 0x%08X, entry 0x%08X, gp 0x%08X, %d constructors\n",
           m.base, m.entry, m.gp, m.nctors);
    fflush(stdout);

    if (getenv("DBZ_DUMP_SEC")) {
        intptr_t secs = atol(getenv("DBZ_DUMP_SEC"));
#ifdef _WIN32
        CreateThread(NULL, 0, watchdog, (LPVOID)secs, 0, NULL);
#else
        pthread_t t;
        pthread_create(&t, NULL, watchdog, (void *)secs);
#endif
    }
    psp_module_start(&m);
    /* DBZ_SHOTS=<dir>: save the picture every 300 frames (about 5 s) for
     * unattended checks. Off by default. */
    const char *shots = getenv("DBZ_SHOTS");
    /* DBZ_PRESS="600:start,1200:cross,...": hold each button for 10 frames at
     * that frame number, for unattended runs. Names: cross circle square
     * triangle start select up down left right l r. */
    struct { uint64_t at; uint32_t b; } press[64];
    int npress = 0;
    if (getenv("DBZ_PRESS")) {
        static const struct { const char *n; uint32_t b; } names[] = {
            {"select", 0x1}, {"start", 0x8}, {"up", 0x10}, {"right", 0x20}, {"down", 0x40},
            {"left", 0x80}, {"l", 0x100}, {"r", 0x200}, {"triangle", 0x1000},
            {"circle", 0x2000}, {"cross", 0x4000}, {"square", 0x8000}};
        char buf[1024];
        snprintf(buf, sizeof buf, "%s", getenv("DBZ_PRESS"));
        for (char *tok = strtok(buf, ","); tok && npress < 64; tok = strtok(NULL, ",")) {
            char *colon = strchr(tok, ':');
            if (!colon) continue;
            *colon = 0;
            for (size_t k = 0; k < sizeof names / sizeof names[0]; k++)
                if (!strcmp(colon + 1, names[k].n)) {
                    press[npress].at = strtoull(tok, NULL, 10);
                    press[npress++].b = names[k].b;
                }
        }
    }
    uint64_t next_shot = 300;
    /* DBZ_DUMP_AT=<frame>: report thread states and recent firmware calls
     * once the game reaches that frame. */
    const uint64_t dump_at = getenv("DBZ_DUMP_AT") ? strtoull(getenv("DBZ_DUMP_AT"), NULL, 10) : 0;
    int dumped = 0;
    while (!psp_window_closed()) {
        psp_threadman_yield();
        if (!dumped && dump_at && psp_display_vblanks() >= dump_at) {
            dumped = 1;
            fprintf(stderr, "[dbz] dump at frame %llu\n", (unsigned long long)psp_display_vblanks());
            psp_threadman_dump(stderr);
            psp_hle_dump_ring(stderr);
            fflush(stderr);
        }
        if (npress) {
            uint64_t v = psp_display_vblanks();
            uint32_t b = 0;
            for (int i = 0; i < npress; i++)
                if (v >= press[i].at && v < press[i].at + 10) b |= press[i].b;
            psp_ctrl_script = b;
        }
        if (shots && psp_display_vblanks() >= next_shot) {
            char path[1024];
            snprintf(path, sizeof path, "%s/frame_%06llu.ppm", shots, (unsigned long long)next_shot);
            psp_display_capture(path);
            next_shot += 300;
        }
    }
    fflush(stdout);
    _Exit(0);
}
