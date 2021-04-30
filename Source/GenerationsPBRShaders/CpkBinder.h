#pragma once

class CpkBinder
{
    static bool enabled;

public:
    static std::vector<std::string> cpkPaths;

    static void applyPatches(ModInfo* info);
};
