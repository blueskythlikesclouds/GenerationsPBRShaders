#pragma once

class Configuration
{
public:
    static uint32_t rlrResolution;

    static bool load(const std::string& filePath);
};
