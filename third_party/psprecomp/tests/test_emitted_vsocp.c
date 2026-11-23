#include "psprecomp/cpu.h"
#include "psprecomp/dispatch.h"
#include "psprecomp/vfpu.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
void psp_recomp_register(void);
static int failures, cases;
static const struct { float input, first, second; } samples[] = {
    {-2, 1, 0}, {-0.0f, 1, 0}, {0, 1, 0}, {0.125f, 0.875f, 0.125f},
    {0.5f, 0.5f, 0.5f}, {1, 0, 1}, {3, 0, 1},
    {INFINITY, 0, 1}, {-INFINITY, 1, 0}, {NAN, NAN, NAN}
};

static void execute(unsigned fn, unsigned a, unsigned b) {
    unsigned size = fn < 3 ? 1 : 2, dest = fn % 3;
    /* Independent expected lane locations, rather than using the helper
     * under test to derive its own expected answer. */
    static const int pair[3][4] = {{4,5,0,0}, {0,1,0,0}, {0,32,0,0}};
    static const int quad[3][4] = {{4,5,6,7}, {0,1,2,3}, {0,32,64,96}};
    const int *out = size == 1 ? pair[dest] : quad[dest];
    psp_cpu_reset(); psp_vfpu_reset();
    float expected[128];
    for (int i = 0; i < 128; i++) psp_cpu.v[i] = expected[i] = 2;
    psp_cpu.v[0] = expected[0] = samples[a].input;
    psp_cpu.v[1] = expected[1] = samples[b].input;
    expected[out[0]] = samples[a].first; expected[out[1]] = samples[a].second;
    if (size == 2) {
        expected[out[2]] = samples[b].first; expected[out[3]] = samples[b].second;
    }
    psp_call_from_host(0x08806000u + fn * 12);
    cases++;
    for (int i = 0; i < 128; i++) {
        float got = psp_cpu.v[i], want = expected[i];
        int written = 0;
        for (unsigned j = 0; j < size * 2; j++) if (out[j] == i) written = 1;
        int good = isnan(want) ? isnan(got) : got == want;
        if (written && want == 0 && signbit(got)) good = 0;
        if (!good) {
            if (failures < 8) fprintf(stderr, "vsocp fn%u inputs%u/%u lane%d: %g expected %g\n",
                                      fn, a, b, i, got, want);
            failures++; break;
        }
    }
}

int main(void) {
    psp_recomp_register();
    for (unsigned fn = 0; fn < 6; fn++) for (unsigned a = 0; a < 10; a++)
        for (unsigned b = 0; b < (fn < 3 ? 1u : 10u); b++) execute(fn, a, b);
    /* Unsupported widths/prefix combinations must remain loud and leave
     * their destinations untouched; no silently guessed prefix behaviour. */
    for (unsigned n = 0; n < 3; n++) {
        psp_cpu_reset(); psp_vfpu_reset();
        for (int i = 0; i < 128; i++) psp_cpu.v[i] = 2;
        uint64_t traps = psp_vfpu_trap_count();
        if (n == 2) psp_vfpu_set_prefix(0, 0xE4);
        psp_call_from_host(0x08806000u + (n < 2 ? 6 + n : 0) * 12);
        if (psp_vfpu_trap_count() != traps + 1 || psp_vfpu_prefix_pending()) failures++;
        for (int i = 0; i < 128; i++) if (psp_cpu.v[i] != 2) { failures++; break; }
    }
    printf("vsocp execution: %d cases, %d failures\n", cases, failures);
    psp_dispatch_reset();
    return failures != 0;
}
