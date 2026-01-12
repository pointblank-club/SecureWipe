#include "wipe/wipe_common.h"

OverwritePlan make_guttmann_plan()
{
    OverwritePlan p;

    for (int i = 0; i < 4; i++)
        p.passes.push_back({OverwritePatternType::RANDOM, 0});

    // Guttmann fixed patterns
    uint8_t patterns[] = {
        0x55, 0xAA,
        0x92, 0x49, 0x24,
        0x00, 0x11, 0x22, 0x33,
        0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB,
        0xCC, 0xDD, 0xEE, 0xFF,
        0x92, 0x49, 0x24,
        0x6D, 0xB6, 0xDB};

    for (uint8_t v : patterns)
        p.passes.push_back({OverwritePatternType::FIXED, v});

    for (int i = 0; i < 4; i++)
        p.passes.push_back({OverwritePatternType::RANDOM, 0});

    return p;
}
