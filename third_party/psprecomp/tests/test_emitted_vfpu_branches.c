#include "psprecomp/cpu.h"
#include "psprecomp/dispatch.h"
#include "psprecomp/vfpu.h"
#include <stdio.h>

void psp_recomp_register(void);

int main(void) {
    int failures = 0;
    psp_recomp_register();
    /* All six meaningful bits, including any/all, and every combination of
     * them. Test both ordinary and likely branches, true and false. */
    for (unsigned n = 0; n < 48; n++) {
        unsigned variant = n / 24, kind = (n / 6) % 4, selector = n % 6;
        for (unsigned cc = 0; cc < 64; cc++) {
            psp_cpu_reset(); psp_vfpu_reset();
            psp_cpu.vfpu_cc = cc;
            unsigned taken = ((cc >> selector) & 1) == (kind & 1);
            unsigned slot = taken || kind < 2;
            unsigned want = (taken ? 4 : 2) + (variant ? 0 : slot);
            unsigned want_cc = variant && slot ? ((kind & 1) ? 0 : 63) : cc;
            psp_call_from_host(0x08805000u + n * 24);
            if (psp_cpu.r[PSP_REG_V0] != want || psp_cpu.vfpu_cc != want_cc) {
                if (failures < 8)
                    fprintf(stderr, "variant %u kind %u bit %u cc %u: result %u/%u, cc %u/%u\n",
                            variant, kind, selector, cc, psp_cpu.r[PSP_REG_V0], want,
                            psp_cpu.vfpu_cc, want_cc);
                failures++;
            }
        }
    }
    printf("VFPU branch execution: %d failures across 3072 cases\n", failures);
    psp_dispatch_reset();
    return failures != 0;
}
