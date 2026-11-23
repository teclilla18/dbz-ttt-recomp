/* Execute translated synthetic instructions. Expected values come from
 * mathematical definitions, independently of the runtime's constant table. */
#include "psprecomp/cpu.h"
#include "psprecomp/dispatch.h"
#include "psprecomp/vfpu.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

void psp_recomp_register(void);

int main(void) {
    const double pi = acos(-1.0);
    const float expected[] = {
        0, FLT_MAX, (float)sqrt(2), (float)sqrt(0.5),
        (float)(2 / sqrt(pi)), (float)(2 / pi), (float)(1 / pi),
        (float)(pi / 4), (float)(pi / 2), (float)pi, (float)exp(1),
        (float)(1 / log(2)), (float)(1 / log(10)), (float)log(2),
        (float)log(10), (float)(2 * pi), (float)(pi / 6),
        (float)log10(2), (float)log2(10), (float)(sqrt(3) / 2)
    };
    /* Independent lane expectations for four different destination matrices. */
    static const int lanes[] = {127, 14, 15, 8, 40, 72, 4, 5, 6, 7};
    int failures = 0;
    psp_recomp_register();
    for (unsigned k = 1; k < sizeof expected / sizeof expected[0]; k++) {
        psp_cpu_reset();
        psp_vfpu_reset();
        for (int i = 0; i < 128; i++) psp_cpu.v[i] = -123;
        psp_call_from_host(0x08804000u + k * 24);
        for (int i = 0; i < 128; i++) {
            int written = 0;
            for (unsigned j = 0; j < sizeof lanes / sizeof lanes[0]; j++)
                if (i == lanes[j]) written = 1;
            float want = written ? expected[k] : -123;
            float got = psp_cpu.v[i];
            if (!isfinite(got) || fabs((double)got - want) > fabs((double)want) * 2e-7) {
                fprintf(stderr, "constant %u lane %d: got %.9g, want %.9g\n", k, i, got, want);
                failures++;
            }
        }
    }
    psp_dispatch_reset();
    return failures != 0;
}
