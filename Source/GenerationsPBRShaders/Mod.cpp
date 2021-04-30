#include "ArchiveTreePatcher.h"
#include "CompressionHandler.h"
#include "CpkBinder.h"
#include "RenderDataManager.h"
#include "LUTHandler.h"
#include "PBROnVanillaFunsies.h"
#include "SceneEffect.h"
#include "GIHandler.h"
#include "LightShaftHandler.h"
#include "ObjectVisualPatcher.h"
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

extern "C" void __declspec(dllexport) Init(ModInfo* info)
{
#if _DEBUG
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
#endif 

    GIHandler::applyPatches();
    ShaderHandler::applyPatches();
    ShadowHandler::applyPatches();
    YggdrasillPatcher::applyPatches();
    VertexBufferHandler::applyPatches();
    SceneEffect::applyPatches();
    CompressionHandler::applyPatches();
    LightShaftHandler::applyPatches();
    RenderDataManager::applyPatches();
    ObjectVisualPatcher::applyPatches();
    ArchiveTreePatcher::applyPatches();
    //CpkBinder::applyPatches(info);

    // PBROnVanillaFunsies::applyPatches();
}

extern "C" void __declspec(dllexport) PostInit()
{
    if (GetModuleHandle(TEXT("BetterFxPipeline.dll")) == nullptr ||
        GetModuleHandle(TEXT("GenerationsD3D9Ex.dll")) == nullptr)
    {
        MessageBox(nullptr, TEXT("This mod requires latest versions of Better FxPipeline and Direct3D 9 Ex enabled."), TEXT("PBR Shaders"), MB_OK | MB_ICONERROR);
        exit(-1);
    }

    // Do these in PostInit to not conflict with other mods.
    LUTHandler::applyPatches();

    WRITE_MEMORY(0x1AD99D0, char*, "shader_vanilla.ar");
    WRITE_MEMORY(0x1AD99D4, char*, "shader_pbr.ar");
    WRITE_MEMORY(0x1AD99E8, char*, "shader_vanilla.arl");
    WRITE_MEMORY(0x1AD99EC, char*, "shader_pbr.arl");
}