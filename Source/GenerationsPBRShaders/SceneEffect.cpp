﻿#include "SceneEffect.h"

DebugParam SceneEffect::Debug = { false, false, -1, -1, -1 };
GIParam SceneEffect::GI = { true, false, 1 };
SGGIParam SceneEffect::SGGI = { 0.7, 0.35 };
ESMParam SceneEffect::ESM = { 4096 };
RLRParam SceneEffect::RLR = { false, 32, 0.8f, 50.0f, 0.1f, 1.0f, 1.0f, 1.0f };

HOOK(void, __cdecl, InitializeSceneEffectParameterFile, 0xD192C0, Sonic::CParameterFile* This)
{
    boost::shared_ptr<Sonic::CParameterGroup> spParameterGroup;
    This->CreateParameterGroup(spParameterGroup, "PBR", "PBR");

    Sonic::CParameterCategory* pDebugParamCategory = spParameterGroup->CreateParameterCategory("Debug", "Debug");
    pDebugParamCategory->CreateParamBool(&SceneEffect::Debug.UseWhiteAlbedo, "UseWhiteAlbedo");
    pDebugParamCategory->CreateParamBool(&SceneEffect::Debug.UseFlatNormal, "UseFlatNormal");
    pDebugParamCategory->CreateParamFloat(&SceneEffect::Debug.FresnelFactorOverride, "FresnelFactorOverride");
    pDebugParamCategory->CreateParamFloat(&SceneEffect::Debug.RoughnessOverride, "RoughnessOverride");
    pDebugParamCategory->CreateParamFloat(&SceneEffect::Debug.MetalnessOverride, "MetalnessOverride");

    spParameterGroup->Flush();

    Sonic::CParameterCategory* pGIParamCategory = spParameterGroup->CreateParameterCategory("GI", "GI");
    pGIParamCategory->CreateParamBool(&SceneEffect::GI.EnableCubicFilter, "EnableCubicFilter");
    pGIParamCategory->CreateParamBool(&SceneEffect::GI.EnableInverseToneMap, "EnableInverseToneMap");
    pGIParamCategory->CreateParamFloat(&SceneEffect::GI.InverseToneMapFactor, "InverseToneMapFactor");

    spParameterGroup->Flush();

    Sonic::CParameterCategory* pSGGIParamCategory = spParameterGroup->CreateParameterCategory("SGGI", "SGGI");
    pSGGIParamCategory->CreateParamFloat(&SceneEffect::SGGI.StartSmoothness, "StartSmoothness");
    pSGGIParamCategory->CreateParamFloat(&SceneEffect::SGGI.EndSmoothness, "EndSmoothness");

    spParameterGroup->Flush();

    Sonic::CParameterCategory* pESMParamCategory = spParameterGroup->CreateParameterCategory("ESM", "ESM");
    pESMParamCategory->CreateParamFloat(&SceneEffect::ESM.Factor, "Factor");

    spParameterGroup->Flush();

    Sonic::CParameterCategory* pRLRParamCategory = spParameterGroup->CreateParameterCategory("RLR", "RLR");
    pRLRParamCategory->CreateParamBool(&SceneEffect::RLR.Enable, "Enable");
    pRLRParamCategory->CreateParamUnsignedLong(&SceneEffect::RLR.StepCount, "StepCount");
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.MaxRoughness, "MaxRoughness");
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.RayLength, "RayLength");
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.Fade, "Fade");
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.Thickness, "Thickness");
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.Saturation, "Saturation");
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.Brightness, "Brightness");

    spParameterGroup->Flush();

    originalInitializeSceneEffectParameterFile(This);
}

void SceneEffect::applyPatches()
{
    INSTALL_HOOK(InitializeSceneEffectParameterFile);
}
