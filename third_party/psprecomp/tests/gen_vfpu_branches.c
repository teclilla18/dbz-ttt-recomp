/* Original synthetic branch programs. Each path returns a distinct value. */
#include "analyze.h"
#include "emit.h"
#include <stdint.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    uint8_t code[48 * 24];
    uint32_t seeds[48];
    for (unsigned n = 0; n < 48; n++) {
        unsigned variant = n / 24, kind = (n / 6) % 4, cc = n % 6;
        uint32_t words[] = {
            0x49000003u | (kind << 16) | (cc << 18),
            /* Second variant changes CC in the slot, after the branch has
             * sampled it. FL for true branches, TR for false branches. */
            variant ? 0x6C008080u | ((kind & 1) ? 0 : 4) : 0x24420001u,
            0x03E00008u, 0x24420002u,
            0x03E00008u, 0x24420004u
        };
        seeds[n] = 0x08805000u + n * 24;
        for (unsigned i = 0; i < 6; i++)
            for (unsigned b = 0; b < 4; b++)
                code[n * 24 + i * 4 + b] = (uint8_t)(words[i] >> (b * 8));
    }
    a_analysis an;
    memset(&an, 0, sizeof an);
    an.code = code; an.base = 0x08805000u; an.size = sizeof code;
    if (a_discover(&an, seeds, 48) != 0) return 1;
    emit_opts opts = {0};
    opts.outdir = argv[1]; opts.prefix = "t_vbranch"; opts.module = "synthetic";
    int result = a_emit(&an, &opts);
    a_analysis_free(&an);
    return result != 0;
}
