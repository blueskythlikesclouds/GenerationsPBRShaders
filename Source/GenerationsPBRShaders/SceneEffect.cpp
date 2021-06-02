#include "SceneEffect.h"

DebugParam SceneEffect::debug = { false, false, -1, -1, -1, -1, DEBUG_VIEW_MODE_NONE, false, false, false, false, false, false, 24 };
CullingParam SceneEffect::culling = { 500, 2500, 100 };
SGGIParam SceneEffect::sggi = { 0.7f, 0.35f };
ESMParam SceneEffect::esm = { 4096 };
RLRParam SceneEffect::rlr = { false, 32, 0.8f, 10000.0f, 0.1f, 0.001f, 1.0f, 1.0f, -1 };
HighlightParam SceneEffect::highlight = { true, 90, 4, 0.08f, 2, 0.02f, 0.3f };
SSAOParam SceneEffect::ssao = { false, 32, 0.25f, 0.25f, 1.0f };

HOOK(void, __cdecl, InitializeSceneEffectParameterFile, 0xD192C0, Sonic::CParameterFile* This)
{
    boost::shared_ptr<Sonic::CParameterGroup> parameterGroup;
    This->CreateParameterGroup(parameterGroup, "PBR", "PBR");

    Sonic::CParameterCategory* debugParamCategory = parameterGroup->CreateParameterCategory("Debug", "Debug");
    debugParamCategory->CreateParamBool(&SceneEffect::debug.useWhiteAlbedo, "UseWhiteAlbedo");
    debugParamCategory->CreateParamBool(&SceneEffect::debug.useFlatNormal, "UseFlatNormal");
    debugParamCategory->CreateParamFloat(&SceneEffect::debug.reflectanceOverride, "ReflectanceOverride");
    debugParamCategory->CreateParamFloat(&SceneEffect::debug.roughnessOverride, "RoughnessOverride");
    debugParamCategory->CreateParamFloat(&SceneEffect::debug.metalnessOverride, "MetalnessOverride");
    debugParamCategory->CreateParamFloat(&SceneEffect::debug.giShadowMapOverride, "GIShadowMapOverride");
    debugParamCategory->CreateParamTypeList((uint32_t*)&SceneEffect::debug.viewMode, "ViewMode", "ViewMode",
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
    debugParamCategory->CreateParamBool(&SceneEffect::debug.disableDirectLight, "DisableDirectLight");
    debugParamCategory->CreateParamBool(&SceneEffect::debug.disableOmniLight, "DisableOmniLight");
    debugParamCategory->CreateParamBool(&SceneEffect::debug.disableSHLightField, "DisableSHLightField");
    debugParamCategory->CreateParamBool(&SceneEffect::debug.disableDefaultIBL, "DisableDefaultIBL");
    debugParamCategory->CreateParamBool(&SceneEffect::debug.disableIBLProbe, "DisableIBLProbe");
    debugParamCategory->CreateParamBool(&SceneEffect::debug.disableLUT, "DisableLUT");
    debugParamCategory->CreateParamUnsignedLong(&SceneEffect::debug.maxProbeCount, "MaxProbeCount");

    parameterGroup->Flush();

    Sonic::CParameterCategory* sggiParamCategory = parameterGroup->CreateParameterCategory("SGGI", "SGGI");
    sggiParamCategory->CreateParamFloat(&SceneEffect::sggi.startSmoothness, "StartSmoothness");
    sggiParamCategory->CreateParamFloat(&SceneEffect::sggi.endSmoothness, "EndSmoothness");

    parameterGroup->Flush();

    Sonic::CParameterCategory* cullingParamCategory = parameterGroup->CreateParameterCategory("Culling", "Culling");
    cullingParamCategory->CreateParamFloat(&SceneEffect::culling.shLightFieldCullingRange, "SHLightFieldCullingRange");
    cullingParamCategory->CreateParamFloat(&SceneEffect::culling.iblProbeCullingRange, "IBLProbeCullingRange");
    cullingParamCategory->CreateParamFloat(&SceneEffect::culling.localLightCullingRange, "LocalLightCullingRange");

    parameterGroup->Flush();

    Sonic::CParameterCategory* esmParamCategory = parameterGroup->CreateParameterCategory("ESM", "ESM");
    esmParamCategory->CreateParamFloat(&SceneEffect::esm.factor, "Factor");

    parameterGroup->Flush();

    Sonic::CParameterCategory* rlrParamCategory = parameterGroup->CreateParameterCategory("RLR", "RLR");
    rlrParamCategory->CreateParamBool(&SceneEffect::rlr.enable, "Enable");
    rlrParamCategory->CreateParamUnsignedLong(&SceneEffect::rlr.stepCount, "StepCount");
    rlrParamCategory->CreateParamFloat(&SceneEffect::rlr.maxRoughness, "MaxRoughness");
    rlrParamCategory->CreateParamFloat(&SceneEffect::rlr.rayLength, "RayLength");
    rlrParamCategory->CreateParamFloat(&SceneEffect::rlr.fade, "Fade");
    rlrParamCategory->CreateParamFloat(&SceneEffect::rlr.accuracyThreshold, "AccuracyThreshold");
    rlrParamCategory->CreateParamFloat(&SceneEffect::rlr.saturation, "Saturation");
    rlrParamCategory->CreateParamFloat(&SceneEffect::rlr.brightness, "Brightness");
    rlrParamCategory->CreateParamInt(&SceneEffect::rlr.maxLod, "MaxLod");

    parameterGroup->Flush();

    Sonic::CParameterCategory* highlightParamCategory = parameterGroup->CreateParameterCategory("Highlight", "Highlight");
    highlightParamCategory->CreateParamBool(&SceneEffect::highlight.enable, "Enable");
    highlightParamCategory->CreateParamFloat(&SceneEffect::highlight.threshold, "Threshold");
    highlightParamCategory->CreateParamFloat(&SceneEffect::highlight.objectAmbientScale, "ObjectAmbientScale");
    highlightParamCategory->CreateParamFloat(&SceneEffect::highlight.objectAlbedoHeighten, "ObjectAlbedoHeighten");
    highlightParamCategory->CreateParamFloat(&SceneEffect::highlight.charaAmbientScale, "CharaAmbientScale");
    highlightParamCategory->CreateParamFloat(&SceneEffect::highlight.charaAlbedoHeighten, "CharaAlbedoHeighten");
    highlightParamCategory->CreateParamFloat(&SceneEffect::highlight.charaFalloffScale, "CharaFalloffScale");

    parameterGroup->Flush();

    Sonic::CParameterCategory* ssaoParamCategory = parameterGroup->CreateParameterCategory("SSAO", "SSAO");
    ssaoParamCategory->CreateParamBool(&SceneEffect::ssao.enable, "Enable");
    ssaoParamCategory->CreateParamUnsignedLong(&SceneEffect::ssao.sampleCount, "SampleCount");
    ssaoParamCategory->CreateParamFloat(&SceneEffect::ssao.radius, "Radius");
    ssaoParamCategory->CreateParamFloat(&SceneEffect::ssao.distanceFade, "DistanceFade");
    ssaoParamCategory->CreateParamFloat(&SceneEffect::ssao.strength, "Strength");

    parameterGroup->Flush();

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
    if (SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_NONE)
        return originalExecuteFxPipelineJobs(This, Edx, A2);

    // Disable post processing when debug view mode in on.
    std::array<bool, POST_PROCESSING_TOGGLES.size()> toggleHistory{};

    // Keep tonemapping on for GIOnly/RLR.
    const size_t index = 
        SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_GI_ONLY ||
        SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_RLR ? 1 : 0;

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
