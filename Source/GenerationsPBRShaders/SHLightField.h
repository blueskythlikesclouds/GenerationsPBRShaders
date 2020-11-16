#pragma once

struct SHLightField
{
    const char* Name;
    INSERT_PADDING(0x4);

    uint32_t ProbeCounts[3];

    float Position[3];
    float Rotation[3];
    float Scale[3];
};

ASSERT_OFFSETOF(SHLightField, Name, 0x0);
ASSERT_OFFSETOF(SHLightField, ProbeCounts, 0x8);
ASSERT_OFFSETOF(SHLightField, Position, 0x14);
ASSERT_OFFSETOF(SHLightField, Rotation, 0x20);
ASSERT_OFFSETOF(SHLightField, Scale, 0x2C);
ASSERT_SIZEOF(SHLightField, 0x38);

struct SHLightFieldSet
{
    INSERT_PADDING(0x94);
    uint32_t SHLFCount;
    SHLightField* SHLFs;
    INSERT_PADDING(0x4);
};

ASSERT_OFFSETOF(SHLightFieldSet, SHLFCount, 0x94);
ASSERT_OFFSETOF(SHLightFieldSet, SHLFs, 0x98);
ASSERT_SIZEOF(SHLightFieldSet, 0xA0);