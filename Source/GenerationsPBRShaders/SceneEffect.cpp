#include "SceneEffect.h"

DebugParam SceneEffect::Debug = { false, false, -1, -1, -1, -1 };
GIParam SceneEffect::GI = { true, false, 1 };
SGGIParam SceneEffect::SGGI = { 0.7, 0.35 };
ESMParam SceneEffect::ESM = { 4096 };
RLRParam SceneEffect::RLR = { false, 32, 0.8f, 10000.0f, 0.1f, 50.0f, 0.95f, 1.0f, 1.0f, -1 };
HighlightParam SceneEffect::Highlight = { true, 90, 4, 0.08, 2, 0.02, 0.3 };

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
    pDebugParamCategory->CreateParamFloat(&SceneEffect::Debug.GIShadowMapOverride, "GIShadowMapOverride");

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
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.AngleExponent, "AngleExponent");
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.AngleThreshold, "AngleThreshold");
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.Saturation, "Saturation");
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.Brightness, "Brightness");
    pRLRParamCategory->CreateParamInt(&SceneEffect::RLR.MaxLod, "MaxLod");

    spParameterGroup->Flush();

    Sonic::CParameterCategory* pHighlightParamCategory = spParameterGroup->CreateParameterCategory("Highlight", "Highlight");
    pHighlightParamCategory->CreateParamBool(&SceneEffect::Highlight.Enable, "Enable");
    pHighlightParamCategory->CreateParamFloat(&SceneEffect::Highlight.Threshold, "Threshold");
    pHighlightParamCategory->CreateParamFloat(&SceneEffect::Highlight.ObjectAmbientScale, "ObjectAmbientScale");
    pHighlightParamCategory->CreateParamFloat(&SceneEffect::Highlight.ObjectAlbedoHeighten, "ObjectAlbedoHeighten");
    pHighlightParamCategory->CreateParamFloat(&SceneEffect::Highlight.CharaAmbientScale, "CharaAmbientScale");
    pHighlightParamCategory->CreateParamFloat(&SceneEffect::Highlight.CharaAlbedoHeighten, "CharaAlbedoHeighten");
    pHighlightParamCategory->CreateParamFloat(&SceneEffect::Highlight.CharaFalloffScale, "CharaFalloffScale");

    spParameterGroup->Flush();

    originalInitializeSceneEffectParameterFile(This);
}

void SceneEffect::applyPatches()
{
    INSTALL_HOOK(InitializeSceneEffectParameterFile);
}
