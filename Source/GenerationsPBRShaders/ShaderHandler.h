#pragma once

class ShaderHandler
{
    static bool enabled;
    
public:
    static void applyPatches();
};

extern size_t shaderCompilerWarmUpIndex;