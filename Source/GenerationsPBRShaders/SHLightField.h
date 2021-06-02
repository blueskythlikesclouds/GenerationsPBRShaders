#pragma once

struct SHLightField
{
    const char* name;
    INSERT_PADDING(0x4);

    uint32_t probeCounts[3];

    float position[3];
    float rotation[3];
    float scale[3];
};

ASSERT_OFFSETOF(SHLightField, name, 0x0);
ASSERT_OFFSETOF(SHLightField, probeCounts, 0x8);
ASSERT_OFFSETOF(SHLightField, position, 0x14);
ASSERT_OFFSETOF(SHLightField, rotation, 0x20);
ASSERT_OFFSETOF(SHLightField, scale, 0x2C);
ASSERT_SIZEOF(SHLightField, 0x38);

struct SHLightFieldSet
{
    INSERT_PADDING(0x94);
    uint32_t shlfCount;
    SHLightField* shlfs;
    INSERT_PADDING(0x4);
};

ASSERT_OFFSETOF(SHLightFieldSet, shlfCount, 0x94);
ASSERT_OFFSETOF(SHLightFieldSet, shlfs, 0x98);
ASSERT_SIZEOF(SHLightFieldSet, 0xA0);