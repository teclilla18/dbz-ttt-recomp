/* Original synthetic GE programs: relative vertices, indices and list flow.
 * Pixel positions prove which memory was read. No game data. */
#include "psprecomp/hle.h"
#include "psprecomp/mem.h"
#include "psprecomp/cpu.h"
#include <stdio.h>

#define LIST 0x08800000u
#define VERT 0x08810000u
#define FB   0x04000000u
static int failures;
static uint32_t cursor;

static void word(uint32_t at, unsigned op, uint32_t arg) {
    psp_write32(at, (op << 24) | (arg & 0xFFFFFFu));
}
static void emit(unsigned op, uint32_t arg) { word(cursor, op, arg); cursor += 4; }
static uint32_t call(uint32_t nid, uint32_t a0, uint32_t a1) {
    psp_cpu.r[4] = a0; psp_cpu.r[5] = a1;
    psp_cpu.r[6] = psp_cpu.r[7] = 0;
    psp_hle_call(nid);
    return psp_cpu.r[2];
}
static void vertex(uint32_t at, int x, int y, uint32_t color) {
    psp_write32(at, color);
    psp_write16(at + 4, (uint16_t)x); psp_write16(at + 6, (uint16_t)y);
    psp_write16(at + 8, 0); psp_write16(at + 10, 0);
}
static void begin(void) {
    psp_ge_reset(); cursor = LIST;
    for (unsigned i = 0; i < 480 * 272; i++) psp_write32(FB + i * 4, 0);
    emit(0x9C, 0); emit(0x9D, 480); emit(0xD2, 3);
    emit(0x12, (7u << 2) | (2u << 7) | (1u << 23));
    vertex(VERT, 10, 20, 0xFF00FF00u);
    vertex(VERT + 12, 14, 24, 0xFF00FF00u);
}
static void finish(void) { emit(0x0F, 0); emit(0x0C, 0); }
static void check_box(const char *what) {
    for (int y = 19; y <= 24; y++) for (int x = 9; x <= 14; x++) {
        uint32_t want = x >= 10 && x < 14 && y >= 20 && y < 24 ? 0xFF00FF00u : 0;
        uint32_t got = psp_read32(FB + (y * 480 + x) * 4);
        if (got != want) {
            fprintf(stderr, "%s pixel %d,%d: %08X expected %08X\n", what, x, y, got, want);
            failures++; return;
        }
    }
}

static void test_origin(int indexed) {
    begin();
    if (indexed) emit(0x12, (7u << 2) | (2u << 7) | (1u << 23) | (1u << 11));
    uint32_t origin = cursor;
    emit(0x14, 0x123456);  /* argument ignored; current command's address is the origin */
    emit(0x10, 0);
    emit(0x01, VERT - origin);
    if (indexed) {
        psp_write8(VERT + 0x100, 1); psp_write8(VERT + 0x101, 0);
        emit(0x02, VERT + 0x100 - origin);
    }
    emit(0x04, (6u << 16) | 2); finish();
    call(0xAB49E76A, LIST, 0);
    check_box(indexed ? "origin indexed" : "origin vertex");
}

static void test_offset(int base_first, int wrap) {
    begin();
    /* Addition must carry across overlapping low address bits. The high
     * four address bits are discarded after addition. */
    uint32_t offset = wrap ? 0xF00000u : 0x001000u;
    if (base_first) emit(0x10, 0x080000);
    emit(0x13, offset);
    if (!base_first) emit(0x10, 0x080000);
    emit(0x01, wrap ? 0x810000u : 0x710000u);
    emit(0x04, (6u << 16) | 2); finish();
    call(0xAB49E76A, LIST, 0);
    check_box(wrap ? "offset wraps 28 bits" : base_first ? "offset preserves base" : "base preserves offset");
}

static void test_jump(void) {
    begin();
    uint32_t origin = cursor;
    emit(0x14, 0); emit(0x10, 0); emit(0x08, 0x80);
    /* The skipped path is a valid list end, so a wrong target stays bounded. */
    finish(); cursor = origin + 0x80;
    emit(0x01, VERT - origin); emit(0x04, (6u << 16) | 2); finish();
    call(0xAB49E76A, LIST, 0);
    check_box("relative jump");
}

static void test_call(int stall) {
    begin();
    uint32_t origin = cursor;
    emit(0x14, 0); emit(0x10, 0); emit(0x0A, 0x100);
    /* RET restores the caller's offset, but keeps BASE as left by the callee. */
    emit(0x01, VERT - origin);
    emit(0x04, (6u << 16) | 2); finish();
    cursor = origin + 0x100;
    emit(0x14, 0); emit(0x10, 0); emit(0x0A, 0x40);
    emit(0x13, 0); emit(0x10, 0); emit(0x0B, 0); finish();
    uint32_t inner = origin + 0x140;
    cursor = inner;
    emit(0x13, 0); emit(0x0B, 0); finish();
    uint32_t id = call(0xAB49E76A, LIST, stall ? inner + 4 : 0);
    if (stall) call(0xE0D68148, id, 0);
    check_box(stall ? "nested call across stall" : "nested call offset restore");
}

static void test_stall_alias(void) {
    for (int update = 0; update < 2; update++) {
        begin();
        emit(0x10, 0x080000); emit(0x01, VERT & 0xFFFFFFu);
        emit(0x08, (LIST + 0x100) & 0xFFFFFFu);
        cursor = LIST + 0x100;
        emit(0x04, (6u << 16) | 2); finish();
        /* The producer uses an uncached alias, while JUMP resolves to a
         * 28-bit address. They still designate the same stop point. */
        uint32_t id = call(0xAB49E76A, LIST | 0x40000000u,
                           (LIST + (update ? 4 : 0x100)) | 0x40000000u);
        if (update) call(0xE0D68148, id, (LIST + 0x100) | 0x40000000u);
        if (psp_read32(FB + (20 * 480 + 10) * 4) != 0) {
            fprintf(stderr, "aliased stall did not stop before the primitive\n"); failures++;
        }
        call(0xE0D68148, id, 0);
        check_box("aliased stall release");
    }
}

static void test_call_keeps_base(void) {
    begin();
    const uint32_t moved = 0x09010000u;
    vertex(moved, 10, 20, 0xFF00FF00u);
    vertex(moved + 12, 14, 24, 0xFF00FF00u);
    emit(0x10, 0x080000); emit(0x0A, (LIST + 0x100) & 0xFFFFFFu);
    emit(0x01, moved & 0xFFFFFFu); emit(0x04, (6u << 16) | 2); finish();
    cursor = LIST + 0x100;
    emit(0x10, 0x090000); emit(0x0B, 0); finish();
    call(0xAB49E76A, LIST, 0);
    check_box("callee base survives return");
}

int main(void) {
    if (psp_mem_init()) return 1;
    psp_cpu_reset(); psp_hle_init();
    test_origin(0); test_origin(1);
    test_offset(0, 0); test_offset(1, 0); test_offset(0, 1);
    test_jump(); test_call(0); test_call(1); test_stall_alias();
    test_call_keeps_base();
    psp_mem_free();
    printf("GE relative addresses: %d failures\n", failures);
    return failures != 0;
}
