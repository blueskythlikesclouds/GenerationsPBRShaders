#pragma once

struct IBLProbe
{
    float matrix[4][4];
    float position[3];
    float bias;
    const char* name;
    INSERT_PADDING(0x24);
};

ASSERT_OFFSETOF(IBLProbe, matrix, 0x0);
ASSERT_OFFSETOF(IBLProbe, position, 0x40);
ASSERT_OFFSETOF(IBLProbe, bias, 0x4C);
ASSERT_OFFSETOF(IBLProbe, name, 0x50);
ASSERT_SIZEOF(IBLProbe, 0x78);

struct IBLProbeSet
{
    INSERT_PADDING(0x8);
    IBLProbe* probes;
    INSERT_PADDING(0x4);
    uint32_t probeCount;
    INSERT_PADDING(0x4);
};

ASSERT_OFFSETOF(IBLProbeSet, probes, 0x8);
ASSERT_OFFSETOF(IBLProbeSet, probeCount, 0x10);
ASSERT_SIZEOF(IBLProbeSet, 0x18);