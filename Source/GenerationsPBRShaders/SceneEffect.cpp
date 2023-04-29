#include "SceneEffect.h"

DebugParam SceneEffect::debug = { false, false, -1, -1, -1, -1, -Eigen::Vector3f::Ones(), -1, DEBUG_VIEW_MODE_NONE, false, false, false, false, false, false };
CullingParam SceneEffect::culling = { 500, 2500, 100 };
SGGIParam SceneEffect::sggi = { 0.7f, 0.35f };
ESMParam SceneEffect::esm = { 4096 };
RLRParam SceneEffect::rlr = { false, 32, 0.8f, 10000.0f, 0.1f, -1 };
SSAOParam SceneEffect::ssao;
BloomParam SceneEffect::bloom = { BLOOM_TYPE_DEFAULT };
VolumetricLightingParam SceneEffect::volumetricLighting = { false, false, 16, -0.98f, 1.0f, 0.002f };
IBLParam SceneEffect::ibl = { 1.0f, 24 };

HOOK(void, __cdecl, InitializeSceneEffectParameterFile, 0xD192C0, Sonic::CParameterFile* This)
{
    boost::shared_ptr<Sonic::CParameterGroup> parameterGroup;
    This->CreateParameterGroup(parameterGroup, "PBR", "PBR");

    Sonic::CEditParam* debugParam = parameterGroup->CreateParameterCategory("Debug", "Debug");
    debugParam->CreateParamBool(&SceneEffect::debug.useWhiteAlbedo, "UseWhiteAlbedo");
    debugParam->CreateParamBool(&SceneEffect::debug.useFlatNormal, "UseFlatNormal");
    debugParam->CreateParamFloat(&SceneEffect::debug.reflectanceOverride, "ReflectanceOverride");
    debugParam->CreateParamFloat(&SceneEffect::debug.smoothnessOverride, "SmoothnessOverride");
    debugParam->CreateParamFloat(&SceneEffect::debug.ambientOcclusionOverride, "AmbientOcclusionOverride");
    debugParam->CreateParamFloat(&SceneEffect::debug.metalnessOverride, "MetalnessOverride");
    debugParam->CreateParamFloat(&SceneEffect::debug.giColorOverride.x(), "GIColorOverrideR");
    debugParam->CreateParamFloat(&SceneEffect::debug.giColorOverride.y(), "GIColorOverrideG");
    debugParam->CreateParamFloat(&SceneEffect::debug.giColorOverride.z(), "GIColorOverrideB");
    debugParam->CreateParamFloat(&SceneEffect::debug.giShadowMapOverride, "GIShadowMapOverride");
    debugParam->CreateParamTypeList((uint32_t*)&SceneEffect::debug.viewMode, "ViewMode", "ViewMode",
        {
            { "None", DEBUG_VIEW_MODE_NONE },
            { "GIOnly", DEBUG_VIEW_MODE_GI_ONLY },
            { "IBLOnly", DEBUG_VIEW_MODE_IBL_ONLY },
            { "GBuffer1", DEBUG_VIEW_MODE_GBUFFER1 },
            { "GBuffer2", DEBUG_VIEW_MODE_GBUFFER2 },
            { "GBuffer3", DEBUG_VIEW_MODE_GBUFFER3 },
            { "RLR", DEBUG_VIEW_MODE_RLR },
            { "SSAO", DEBUG_VIEW_MODE_SSAO },
            { "VolumetricLighting", DEBUG_VIEW_MODE_VOLUMETRIC_LIGHTING },
            { "ShadowMap", DEBUG_VIEW_MODE_SHADOW_MAP },
            { "ShadowMapNoTerrain", DEBUG_VIEW_MODE_SHADOW_MAP_NO_TERRAIN },
        });
    debugParam->CreateParamBool(&SceneEffect::debug.disableDirectLight, "DisableDirectLight");
    debugParam->CreateParamBool(&SceneEffect::debug.disableLocalLight, "DisableLocalLight");
    debugParam->CreateParamBool(&SceneEffect::debug.disableSHLightField, "DisableSHLightField");
    debugParam->CreateParamBool(&SceneEffect::debug.disableDefaultIBL, "DisableDefaultIBL");
    debugParam->CreateParamBool(&SceneEffect::debug.disableIBLProbe, "DisableIBLProbe");
    debugParam->CreateParamBool(&SceneEffect::debug.disableLUT, "DisableLUT");

    parameterGroup->Flush();

    Sonic::CEditParam* sggiParam = parameterGroup->CreateParameterCategory("SGGI", "SGGI");
    sggiParam->CreateParamFloat(&SceneEffect::sggi.startSmoothness, "StartSmoothness");
    sggiParam->CreateParamFloat(&SceneEffect::sggi.endSmoothness, "EndSmoothness");

    parameterGroup->Flush();

    Sonic::CEditParam* cullingParam = parameterGroup->CreateParameterCategory("Culling", "Culling");
    cullingParam->CreateParamFloat(&SceneEffect::culling.shLightFieldCullingRange, "SHLightFieldCullingRange");
    cullingParam->CreateParamFloat(&SceneEffect::culling.iblProbeCullingRange, "IBLProbeCullingRange");
    cullingParam->CreateParamFloat(&SceneEffect::culling.localLightCullingRange, "LocalLightCullingRange");

    parameterGroup->Flush();

    Sonic::CEditParam* esmParam = parameterGroup->CreateParameterCategory("ESM", "ESM");
    esmParam->CreateParamFloat(&SceneEffect::esm.factor, "Factor");
    esmParam->CreateParamBool(&SceneEffect::esm.renderTerrain, "RenderTerrain");

    parameterGroup->Flush();

    Sonic::CEditParam* rlrParam = parameterGroup->CreateParameterCategory("RLR", "RLR");
    rlrParam->CreateParamBool(&SceneEffect::rlr.enable, "Enable");
    rlrParam->CreateParamUnsignedLong(&SceneEffect::rlr.stepCount, "StepCount");
    rlrParam->CreateParamFloat(&SceneEffect::rlr.maxRoughness, "MaxRoughness");
    rlrParam->CreateParamFloat(&SceneEffect::rlr.rayLength, "RayLength");
    rlrParam->CreateParamFloat(&SceneEffect::rlr.fade, "Fade");
    rlrParam->CreateParamInt(&SceneEffect::rlr.maxLod, "MaxLod");

    parameterGroup->Flush();

    Sonic::CEditParam* ssaoParam = parameterGroup->CreateParameterCategory("SSAO", "SSAO");
    ssaoParam->CreateParamBool(&SceneEffect::ssao.enable, "Enable");
    ssaoParam->CreateParamFloat(&SceneEffect::ssao.Radius, "Radius");
    ssaoParam->CreateParamFloat(&SceneEffect::ssao.Bias, "Bias");
    ssaoParam->CreateParamFloat(&SceneEffect::ssao.PowerExponent, "PowerExponent");
    ssaoParam->CreateParamTypeList((uint32_t*)&SceneEffect::ssao.StepCount, "StepCount", "StepCount",
        {
            {"4 Steps", GFSDK_SSAO_STEP_COUNT_4},
            {"8 Steps", GFSDK_SSAO_STEP_COUNT_8}
        });

    ssaoParam->CreateParamTypeList((uint32_t*)&SceneEffect::ssao.Blur.Radius, "BlurRadius", "BlurRadius",
        {
            {"2 Pixels", GFSDK_SSAO_BLUR_RADIUS_2},
            {"4 Pixels", GFSDK_SSAO_BLUR_RADIUS_4}
        });

    ssaoParam->CreateParamFloat(&SceneEffect::ssao.Blur.Sharpness, "BlurSharpness");

    parameterGroup->Flush();

    Sonic::CEditParam* bloomParam = parameterGroup->CreateParameterCategory("Bloom", "Bloom");
    bloomParam->CreateParamTypeList((uint32_t*)&SceneEffect::bloom.type, "Type", "Type",
        {
            { "Default", BLOOM_TYPE_DEFAULT },
            { "Sonic Colors", BLOOM_TYPE_COLORS },
            { "Sonic Forces", BLOOM_TYPE_FORCES },
            { "Better FxPipeline", BLOOM_TYPE_BFXP },
        });

    parameterGroup->Flush();

    Sonic::CEditParam* volumetricLightingCategory = parameterGroup->CreateParameterCategory("VolumetricLighting", "VolumetricLighting");
    volumetricLightingCategory->CreateParamBool(&SceneEffect::volumetricLighting.enable, "Enable");
    volumetricLightingCategory->CreateParamBool(&SceneEffect::volumetricLighting.ignoreSky, "IgnoreSky");
    volumetricLightingCategory->CreateParamUnsignedLong(&SceneEffect::volumetricLighting.sampleCount, "SampleCount");
    volumetricLightingCategory->CreateParamFloat(&SceneEffect::volumetricLighting.g, "G");
    volumetricLightingCategory->CreateParamFloat(&SceneEffect::volumetricLighting.inScatteringScale, "InScatteringScale");
    volumetricLightingCategory->CreateParamFloat(&SceneEffect::volumetricLighting.depthThreshold, "DepthThreshold");

    parameterGroup->Flush();

    Sonic::CEditParam* iblCategory = parameterGroup->CreateParameterCategory("IBL", "IBL");
    iblCategory->CreateParamFloat(&SceneEffect::ibl.defaultIBLIntensity, "DefaultIBLIntensity");
    iblCategory->CreateParamUnsignedLong(&SceneEffect::ibl.maxIBLProbeCount, "MaxIBLProbeCount");

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

    // Keep tonemapping on for GIOnly/IBLOnly/RLR/VolumetricLighting.
    const size_t index = 
        SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_GI_ONLY ||
        SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_IBL_ONLY ||
        SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_RLR ||
        SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_VOLUMETRIC_LIGHTING ? 1 : 0;

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
