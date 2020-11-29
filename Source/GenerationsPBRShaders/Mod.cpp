#include "LUTHandler.h"
#include "SceneEffect.h"
#include "SGGIHandler.h"
#include "ShaderHandler.h"
#include "ShadowHandler.h"
#include "SRGBHandler.h"
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
    SRGBHandler::applyPatches();
    YggdrasillPatcher::applyPatches();
    VertexBufferHandler::applyPatches();
    SceneEffect::applyPatches();
}