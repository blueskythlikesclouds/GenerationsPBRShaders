#include "SceneEffect.h"

DebugParam SceneEffect::Debug = { false, false, -1, -1, -1, -1, DEBUG_VIEW_MODE_NONE, false, false, false, false, false };
GIParam SceneEffect::GI = { true, false, 1 };
SGGIParam SceneEffect::SGGI = { 0.7f, 0.35f };
ESMParam SceneEffect::ESM = { 4096 };
RLRParam SceneEffect::RLR = { false, 32, 0.8f, 10000.0f, 0.1f, 0.001f, 1.0f, 1.0f, -1 };
HighlightParam SceneEffect::Highlight = { true, 90, 4, 0.08f, 2, 0.02f, 0.3f };
SSAOParam SceneEffect::SSAO = { false, 32, 0.25f, 0.25f, 1.0f };

HOOK(void, __cdecl, InitializeSceneEffectParameterFile, 0xD192C0, Sonic::CParameterFile* This)
{
    boost::shared_ptr<Sonic::CParameterGroup> spParameterGroup;
    This->CreateParameterGroup(spParameterGroup, "PBR", "PBR");

    Sonic::CParameterCategory* pDebugParamCategory = spParameterGroup->CreateParameterCategory("Debug", "Debug");
    pDebugParamCategory->CreateParamBool(&SceneEffect::Debug.UseWhiteAlbedo, "UseWhiteAlbedo");
    pDebugParamCategory->CreateParamBool(&SceneEffect::Debug.UseFlatNormal, "UseFlatNormal");
    pDebugParamCategory->CreateParamFloat(&SceneEffect::Debug.ReflectanceOverride, "ReflectanceOverride");
    pDebugParamCategory->CreateParamFloat(&SceneEffect::Debug.RoughnessOverride, "RoughnessOverride");
    pDebugParamCategory->CreateParamFloat(&SceneEffect::Debug.MetalnessOverride, "MetalnessOverride");
    pDebugParamCategory->CreateParamFloat(&SceneEffect::Debug.GIShadowMapOverride, "GIShadowMapOverride");
    pDebugParamCategory->CreateParamTypeList((uint32_t*)&SceneEffect::Debug.ViewMode, "ViewMode", "ViewMode",
        {
            { "None", DEBUG_VIEW_MODE_NONE },
            { "GIOnly", DEBUG_VIEW_MODE_GI_ONLY },
            { "GBuffer1", DEBUG_VIEW_MODE_GBUFFER1 },
            { "GBuffer2", DEBUG_VIEW_MODE_GBUFFER2 },
            { "GBuffer3", DEBUG_VIEW_MODE_GBUFFER3 },
            { "RLR", DEBUG_VIEW_MODE_RLR },
            { "SSAO", DEBUG_VIEW_MODE_SSAO },
            { "ShadowMap", DEBUG_VIEW_MODE_SHADOW_MAP },
            { "ShadowMapNoTerrain", DEBUG_VIEW_MODE_SHADOW_MAP_NO_TERRAIN },
        });
    pDebugParamCategory->CreateParamBool(&SceneEffect::Debug.DisableDirectLight, "DisableDirectLight");
    pDebugParamCategory->CreateParamBool(&SceneEffect::Debug.DisableOmniLight, "DisableOmniLight");
    pDebugParamCategory->CreateParamBool(&SceneEffect::Debug.DisableSHLightField, "DisableSHLightField");
    pDebugParamCategory->CreateParamBool(&SceneEffect::Debug.DisableDefaultIBL, "DisableDefaultIBL");
    pDebugParamCategory->CreateParamBool(&SceneEffect::Debug.DisableIBLProbe, "DisableIBLProbe");

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
    pRLRParamCategory->CreateParamFloat(&SceneEffect::RLR.AccuracyThreshold, "AccuracyThreshold");
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

    Sonic::CParameterCategory* pSSAOParamCategory = spParameterGroup->CreateParameterCategory("SSAO", "SSAO");
    pSSAOParamCategory->CreateParamBool(&SceneEffect::SSAO.Enable, "Enable");
    pSSAOParamCategory->CreateParamUnsignedLong(&SceneEffect::SSAO.SampleCount, "SampleCount");
    pSSAOParamCategory->CreateParamFloat(&SceneEffect::SSAO.Radius, "Radius");
    pSSAOParamCategory->CreateParamFloat(&SceneEffect::SSAO.DistanceFade, "DistanceFade");
    pSSAOParamCategory->CreateParamFloat(&SceneEffect::SSAO.Strength, "Strength");

    spParameterGroup->Flush();

    originalInitializeSceneEffectParameterFile(This);
}

std::array<bool*, 12> POST_PROCESSING_TOGGLES =
{
    (bool*)0x01A4358E, // HDR
    (bool*)0x01A43102, // BoostBlur3D
    (bool*)0x01AD6198, // BoostBlur2D
    (bool*)0x01A43103, // MotionBlur
    (bool*)0x01A4323D, // BloomStar
    (bool*)0x01E5E333, // LightShaft
    (bool*)0x01A4358D, // DepthOfField
    (bool*)0x01AD6277, // FadeOut
    (bool*)0x01B22E78, // ReflectionMap
    (bool*)0x01B22E8C, // ColorCorrection
    (bool*)0x01A46DE7, // AfterImage
    (bool*)0x01B22EA8, // CrossFade
};

HOOK(void, __fastcall, ExecuteFxPipelineJobs, 0x78A3D0, void* This, void* Edx, void* A2)
{
    if (SceneEffect::Debug.ViewMode == DEBUG_VIEW_MODE_NONE)
        return originalExecuteFxPipelineJobs(This, Edx, A2);

    // Disable post processing when debug view mode in on.
    std::array<bool, POST_PROCESSING_TOGGLES.size()> toggleHistory{};

    // Keep tonemapping on for GIOnly/RLR.
    const size_t index = 
        SceneEffect::Debug.ViewMode == DEBUG_VIEW_MODE_GI_ONLY ||
        SceneEffect::Debug.ViewMode == DEBUG_VIEW_MODE_RLR ? 1 : 0;

    for (size_t i = index; i < POST_PROCESSING_TOGGLES.size(); i++)
    {
        toggleHistory[i] = *POST_PROCESSING_TOGGLES[i];
        *POST_PROCESSING_TOGGLES[i] = false;
    }

    originalExecuteFxPipelineJobs(This, Edx, A2);

    // Restore
    for (size_t i = index; i < POST_PROCESSING_TOGGLES.size(); i++)
        *POST_PROCESSING_TOGGLES[i] = toggleHistory[i];
}

void SceneEffect::applyPatches()
{
    INSTALL_HOOK(InitializeSceneEffectParameterFile);
    INSTALL_HOOK(ExecuteFxPipelineJobs);
}
