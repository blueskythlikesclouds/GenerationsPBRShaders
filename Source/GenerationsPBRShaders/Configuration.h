#pragma once

class Configuration
{
public:
    static uint32_t rlrResolution;
    static uint32_t maxProbeCount;

    static bool load(const std::string& filePath);
};
