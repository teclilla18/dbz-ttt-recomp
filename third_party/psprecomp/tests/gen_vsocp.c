/* Original synthetic vsocp encodings, including overlapping destinations. */
#include "analyze.h"
#include "emit.h"
#include <stdint.h>
#include <string.h>
int main(int argc, char **argv) {
    if (argc != 2) return 1;
    uint8_t code[8 * 12]; uint32_t seeds[8];
    const uint32_t dest[] = {4, 0, 32};
    for (unsigned n = 0; n < 8; n++) {
        unsigned width = n < 6 ? (n / 3 ? 0x80 : 0) : n == 6 ? 0x8000 : 0x8080;
        uint32_t words[] = {0xD0450000u | width | dest[n % 3], 0x03E00008u, 0};
        seeds[n] = 0x08806000u + n * 12;
        for (unsigned i = 0; i < 3; i++) for (unsigned b = 0; b < 4; b++)
            code[n * 12 + i * 4 + b] = (uint8_t)(words[i] >> (b * 8));
    }
    a_analysis an = {0};
    an.code = code; an.base = 0x08806000u; an.size = sizeof code;
    if (a_discover(&an, seeds, 8) != 0) return 1;
    emit_opts opts = {0};
    opts.outdir = argv[1]; opts.prefix = "t_vsocp"; opts.module = "synthetic";
    int result = a_emit(&an, &opts);
    a_analysis_free(&an);
    return result != 0;
}
