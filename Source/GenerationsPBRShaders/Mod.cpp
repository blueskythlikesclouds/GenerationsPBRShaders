#include "BloomHandler.h"
#include "CompressionHandler.h"
#include "CpkBinder.h"
#include "RenderDataManager.h"
#include "LUTHandler.h"
#include "SceneEffect.h"
#include "GIHandler.h"
#include "IBLCaptureService.h"
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

#ifdef ENABLE_IBL_CAPTURE_SERVICE
    static size_t index = 0;

    const std::unique_ptr<DirectX::ScratchImage> result = IBLCaptureService::getResultIfReady();
    if (result != nullptr)
    {
        wchar_t fileNameWideChar[MAX_PATH];
        std::string fileNameMultiByte;

        if (IBLCaptureService::mode == IBLCaptureMode::DefaultIBL)
            fileNameMultiByte = "work/ibl/" + StageId::get() + "_defaultibl.dds";
        else
            fileNameMultiByte = "work/ibl/" + RenderDataManager::iblProbes[index]->name + ".dds";

        MultiByteToWideChar(CP_UTF8, 0, fileNameMultiByte.c_str(), -1, fileNameWideChar, MAX_PATH);

        DirectX::SaveToDDSFile(result->GetImages(), result->GetImageCount(), result->GetMetadata(), DirectX::DDS_FLAGS_NONE, fileNameWideChar);

        if (IBLCaptureService::mode == IBLCaptureMode::IBLProbe)
        {
            index++;

            if (index == RenderDataManager::iblProbes.size())
                index = 0;
        }
    }

    if (!RenderDataManager::iblProbes.empty() && (index > 0 || (GetAsyncKeyState(VK_F1) & 1) != 0))
        IBLCaptureService::capture(RenderDataManager::iblProbes[index]->position, 128, IBLCaptureMode::IBLProbe);
    else if ((GetAsyncKeyState(VK_F2) & 1) != 0)
        IBLCaptureService::capture(Eigen::AlignedVector3f::Zero(), 512, IBLCaptureMode::DefaultIBL);
#endif
}

extern "C" void __declspec(dllexport) Init(ModInfo* info)
{
#if _DEBUG
    if (!GetConsoleWindow())
        AllocConsole();

    freopen("CONOUT$", "w", stdout);
#endif

    std::string dir = info->CurrentMod->Path;

    size_t pos = dir.find_last_of("\\/");
    if (pos != std::string::npos)
        dir.erase(pos + 1);

    if (!Configuration::load(dir + "GenerationsPBRShaders.ini"))
        MessageBox(NULL, L"Failed to parse GenerationsPBRShaders.ini", NULL, MB_ICONERROR);

    BloomHandler::applyPatches();
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

#ifdef ENABLE_IBL_CAPTURE_SERVICE
    IBLCaptureService::applyPatches();
#endif
}

extern "C" void __declspec(dllexport) PostInit(ModInfo* info)
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