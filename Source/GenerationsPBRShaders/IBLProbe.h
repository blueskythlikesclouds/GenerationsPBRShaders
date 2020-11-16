#pragma once

struct IBLProbe
{
    float Matrix[4][4];
    float Position[3];
    float Bias;
    const char* Name;
    INSERT_PADDING(0x24);
};

ASSERT_OFFSETOF(IBLProbe, Matrix, 0x0);
ASSERT_OFFSETOF(IBLProbe, Position, 0x40);
ASSERT_OFFSETOF(IBLProbe, Bias, 0x4C);
ASSERT_OFFSETOF(IBLProbe, Name, 0x50);
ASSERT_SIZEOF(IBLProbe, 0x78);

struct IBLProbeSet
{
    INSERT_PADDING(0x8);
    IBLProbe* Probes;
    INSERT_PADDING(0x4);
    uint32_t ProbeCount;
    INSERT_PADDING(0x4);
};

ASSERT_OFFSETOF(IBLProbeSet, Probes, 0x8);
ASSERT_OFFSETOF(IBLProbeSet, ProbeCount, 0x10);
ASSERT_SIZEOF(IBLProbeSet, 0x18);