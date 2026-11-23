/* Original boundary regression probe, linked only with the external analyzer.
 * No game input. Run with AddressSanitizer as well as the ordinary host build. */
#include "analyze.h"
#include <stdio.h>
#include <string.h>

static int failures;
static int checks;
#define CHECK(expr) do { ++checks; if (!(expr)) { ++failures; \
    fprintf(stderr, "FAIL boundary check at line %d\n", __LINE__); } } while (0)

int main(void) {
    unsigned char code[8] = {0};
    a_analysis an;
    memset(&an, 0, sizeof an);
    an.code = code;
    an.base = 0x08900000;
    for (unsigned size = 0; size <= 8; ++size) {
        an.size = size;
        CHECK(a_in_range(&an, an.base) == (size >= 4));
        CHECK(a_in_range(&an, an.base + 4) == (size >= 8));
        CHECK(!a_in_range(&an, an.base - 4));
        CHECK(!a_in_range(&an, an.base + 1));
    }
    an.base = 0xFFFFFFFCu;
    an.size = 4;
    CHECK(a_in_range(&an, an.base));
    CHECK(!a_in_range(&an, 0));
    if (failures) {
        printf("%d/%d boundary checks failed; discovery probe skipped.\n", failures, checks);
        return 1;
    }
    an.base = 0x08900000;
    an.size = 5;
    unsigned partial = an.base + 4;
    CHECK(a_discover(&an, &partial, 1) == 0);
    CHECK(an.nfuncs == 0 && an.bytes_reached == 0);
    a_analysis_free(&an);
    printf("%d boundary checks; %d failures.\n", checks, failures);
    return failures ? 1 : 0;
}
