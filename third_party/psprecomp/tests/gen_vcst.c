/* Original synthetic encodings, not extracted game instructions. Generate C
 * with the production analyzer/emitter, then compile and execute it in CTest. */
#include "analyze.h"
#include "emit.h"
#include <stdint.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    uint8_t code[32 * 24];
    uint32_t seeds[32];
    static const uint32_t widths[] = {0, 0x80, 0x8000, 0x8080};
    static const uint32_t dest[] = {127, 76, 40, 4};
    for (unsigned k = 0; k < 32; k++) {
        seeds[k] = 0x08804000u + k * 24;
        for (unsigned i = 0; i < 6; i++) {
            uint32_t word = i < 4 ? 0xD0600000u | (k << 16) | widths[i] | dest[i]
                                 : i == 4 ? 0x03E00008u : 0;
            unsigned off = k * 24 + i * 4;
            for (unsigned b = 0; b < 4; b++) code[off + b] = (uint8_t)(word >> (b * 8));
        }
    }
    a_analysis an;
    memset(&an, 0, sizeof an);
    an.code = code;
    an.base = 0x08804000u;
    an.size = sizeof code;
    if (a_discover(&an, seeds, 32) != 0) return 1;
    emit_opts opts = {0};
    opts.outdir = argv[1];
    opts.prefix = "t_vcst";
    opts.module = "synthetic";
    int result = a_emit(&an, &opts);
    a_analysis_free(&an);
    return result != 0;
}
