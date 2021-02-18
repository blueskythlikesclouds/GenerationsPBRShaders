#include "ATI2Handler.h"
#include "CompressionHandler.h"
#include "LUTHandler.h"
#include "PBROnVanillaFunsies.h"
#include "SceneEffect.h"
#include "SGGIHandler.h"
#include "ShaderHandler.h"
#include "ShadowHandler.h"
#include "StageId.h"
#include "VertexBufferHandler.h"
#include "YggdrasillPatcher.h"

extern "C" void __declspec(dllexport) OnFrame()
{
    StageId::update();
    PBROnVanillaFunsies::onFrame();
}

extern "C" void __declspec(dllexport) Init()
{
#if _DEBUG
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
#endif 

    SGGIHandler::applyPatches();
    ShaderHandler::applyPatches();
    ShadowHandler::applyPatches();
    YggdrasillPatcher::applyPatches();
    VertexBufferHandler::applyPatches();
    SceneEffect::applyPatches();
    ATI2Handler::applyPatches();
    CompressionHandler::applyPatches();

    // PBROnVanillaFunsies::applyPatches();
}

extern "C" void __declspec(dllexport) PostInit()
{
    if (GetModuleHandle(TEXT("BetterFxPipeline.dll")) == nullptr ||
        GetModuleHandle(TEXT("GenerationsD3D9Ex.dll")) == nullptr)
    {
        MessageBox(nullptr, TEXT("Please have latest versions of Better FxPipeline and Direct3D 9 Ex enabled"), TEXT("PBR Shaders"), MB_OK | MB_ICONERROR);
        exit(-1);
    }

    // Do these in PostInit to not conflict with other mods.
    LUTHandler::applyPatches();

    WRITE_MEMORY(0x1AD99D0, char*, "shader_vanilla.ar");
    WRITE_MEMORY(0x1AD99D4, char*, "shader_pbr.ar");
    WRITE_MEMORY(0x1AD99E8, char*, "shader_vanilla.arl");
    WRITE_MEMORY(0x1AD99EC, char*, "shader_pbr.arl");
}