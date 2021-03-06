﻿#include "Configuration.h"

bool Configuration::rlrEnable = true;
uint32_t Configuration::rlrResolution = 0;
uint32_t Configuration::maxProbeCount = 24;

bool Configuration::load(const std::string& filePath)
{
    const INIReader reader(filePath);
    if (reader.ParseError() != 0)
        return false;

    rlrEnable = reader.GetBoolean("SSR", "Enable", true);
    rlrResolution = (uint32_t)reader.GetInteger("SSR", "Resolution", 0);
    maxProbeCount = (uint32_t)reader.GetInteger("IBLProbe", "MaxProbeCount", 24);

    return true;
}
