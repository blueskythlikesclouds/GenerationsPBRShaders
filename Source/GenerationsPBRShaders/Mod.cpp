#include "LUTHandler.h"
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
}

extern "C" void __declspec(dllexport) Init()
{
#if _DEBUG
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
#endif 

    LUTHandler::applyPatches();
    SGGIHandler::applyPatches();
    ShaderHandler::applyPatches();
    ShadowHandler::applyPatches();
    YggdrasillPatcher::applyPatches();
    VertexBufferHandler::applyPatches();
    SceneEffect::applyPatches();
}

extern "C" void __declspec(dllexport) PostInit()
{
    if (GetModuleHandle(TEXT("BetterFxPipeline.dll")) == nullptr ||
        GetModuleHandle(TEXT("GenerationsD3D9Ex.dll")) == nullptr)
    {
        MessageBox(nullptr, TEXT("Please have latest versions of Better FxPipeline and Direct3D 9 Ex enabled"), TEXT("PBR Shaders"), MB_OK | MB_ICONERROR);
        exit(-1);
    }
}