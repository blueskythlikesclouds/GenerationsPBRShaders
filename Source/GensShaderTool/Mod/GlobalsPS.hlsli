#ifndef GLOBALS_PS_HLSLI_INCLUDED
#define GLOBALS_PS_HLSLI_INCLUDED

Texture2D<float> g_VerticalShadowMapTexture : register(t7);
Texture2D<float4> g_GITexture : register(t10);
Texture2D<float4> g_FramebufferTexture : register(t11);
Texture2D<float> g_DepthTexture : register(t12);
Texture2D<float> g_ShadowMapTexture : register(t13);
Texture2D<float4> g_ReflectionMapTexture : register(t14);
Texture2D<float4> g_ReflectionMap2Texture : register(t15);

Texture2D<float> g_BlueNoiseTexture : register(t16);
Texture2D<float> g_LuminanceTexture : register(t17);
Texture2D<float> g_OcclusionTexture : register(t18);
Texture2D<float> g_SSAOTexture : register(t19);
Texture2D<float2> g_EnvBRDFTexture : register(t20);
Texture2D<float4> g_RLRTexture : register(t21);
Texture2DArray<float3> g_SGGITexture : register(t22);
Texture3D<float4> g_SHLightFieldTextures[3] : register(t23);
TextureCube<float4> g_DefaultIBLTexture : register(t26);
TextureCubeArray<float4> g_IBLProbeTextures : register(t27);

SamplerState g_LinearClampSampler : register(s11);
SamplerState g_PointClampSampler : register(s12);
SamplerState g_PointBorderSampler : register(s13);
SamplerState g_PointRepeatSampler : register(s15);

cbuffer cbRenderData : register(b3)
{
    row_major float3x4 mrgSHLightFieldMatrices[3];
    float3 mrgSHLightFieldParams[3];

    row_major float3x4 mrgIBLProbeMatrices[24];
    float4 mrgIBLProbeParams[24];

    float4 mrgLocalLightData[64];

    float mrgIBLProbeLodParam;
    float mrgDefaultIBLLodParam;

    uint mrgIBLProbeCount;
    uint mrgLocalLightCount;

    bool mrgDefaultIBLExposurePacked;
}

cbuffer cbSceneEffect : register(b2)
{
    float g_ReflectanceOverride;
    float g_RoughnessOverride;
    float g_AmbientOcclusionOverride;
    float g_MetalnessOverride;

    float3 g_GIColorOverride;
    float g_GIShadowOverride;

    float2 g_SGGIParam;
    float g_RcpMiddleGray;
    float g_ESMFactor;

    float2 g_ShadowMapSize;

    bool g_UseWhiteAlbedo;
    bool g_UseFlatNormal;
    bool g_UsePBR;

    float g_DefaultIBLIntensity;
}

#ifndef GLOBALS_ONLY_PBR_CONSTANTS

#define mrgIsEnableHemisphere     (1 << 16)
#define g_IsShadowMapEnable       (1 << 17)
#define g_IsAlphaDepthBlur        (1 << 18)
#define g_IsGIEnabled             (1 << 19)
#define g_IsLightScatteringEnable (1 << 20)
#define g_IsSoftParticle          (1 << 21)

cbuffer cbGlobalsPS : register(b0)
{
    row_major float4x4 g_MtxProjection : packoffset(c0);
    row_major float4x4 g_MtxInvProjection : packoffset(c4);
    float4 mrgGlobalLight_Direction : packoffset(c10);
    float4 mrgGlobalLight_Direction_View : packoffset(c11);
    float4 g_Diffuse : packoffset(c16);
    float4 g_Ambient : packoffset(c17);
    float4 g_Specular : packoffset(c18);
    float4 g_Emissive : packoffset(c19);
    float4 g_PowerGlossLevel : packoffset(c20);
    float4 g_OpacityReflectionRefractionSpectype : packoffset(c21);
    float4 g_EyePosition : packoffset(c22);
    float4 g_EyeDirection : packoffset(c23);
    float4 g_ViewportSize : packoffset(c24);
    float4 g_CameraNearFarAspect : packoffset(c25);
    float4 mrgTexcoordIndex[4] : packoffset(c26);
    float4 mrgAmbientColor : packoffset(c30);
    float4 mrgGroundColor : packoffset(c31);
    float4 mrgSkyColor : packoffset(c32);
    float4 mrgPowerGlossLevel : packoffset(c33);
    float4 mrgEmissionPower : packoffset(c34);
    float4 mrgGlobalLight_Diffuse : packoffset(c36);
    float4 mrgGlobalLight_Specular : packoffset(c37);
    float4 mrgLocalLight0_Position : packoffset(c38);
    float4 mrgLocalLight0_Color : packoffset(c39);
    float4 mrgLocalLight0_Range : packoffset(c40);
    float4 mrgLocalLight0_Attribute : packoffset(c41);
    float4 mrgLocalLight1_Position : packoffset(c42);
    float4 mrgLocalLight1_Color : packoffset(c43);
    float4 mrgLocalLight1_Range : packoffset(c44);
    float4 mrgLocalLight1_Attribute : packoffset(c45);
    float4 mrgLocalLight2_Position : packoffset(c46);
    float4 mrgLocalLight2_Color : packoffset(c47);
    float4 mrgLocalLight2_Range : packoffset(c48);
    float4 mrgLocalLight2_Attribute : packoffset(c49);
    float4 mrgLocalLight3_Position : packoffset(c50);
    float4 mrgLocalLight3_Color : packoffset(c51);
    float4 mrgLocalLight3_Range : packoffset(c52);
    float4 mrgLocalLight3_Attribute : packoffset(c53);
    float4 mrgLocalLight4_Position : packoffset(c54);
    float4 mrgLocalLight4_Color : packoffset(c55);
    float4 mrgLocalLight4_Range : packoffset(c56);
    float4 mrgLocalLight4_Attribute : packoffset(c57);
    float4 mrgEyeLight_Diffuse : packoffset(c58);
    float4 mrgEyeLight_Specular : packoffset(c59);
    float4 mrgEyeLight_Range : packoffset(c60);
    float4 mrgEyeLight_Attribute : packoffset(c61);
    float4 mrgFresnelParam : packoffset(c62);
    float4 mrgLuminanceRange : packoffset(c63);
    float4 mrgInShadowScale : packoffset(c64);
    float4 g_ShadowMapParams : packoffset(c65);
    float4 mrgColourCompressFactor : packoffset(c66);
    float4 g_BackGroundScale : packoffset(c67);
    float4 g_TimeParam : packoffset(c68);
    float4 g_GIModeParam : packoffset(c69);
    float4 g_GI0Scale : packoffset(c70);
    float4 g_GI1Scale : packoffset(c71);
    float4 g_LightScattering_Ray_Mie_Ray2_Mie2 : packoffset(c72);
    float4 g_LightScattering_ConstG_FogDensity : packoffset(c73);
    float4 g_LightScatteringFarNearScale : packoffset(c74);
    float4 g_LightScatteringColor : packoffset(c75);
    float4 g_LightScatteringMode : packoffset(c76);
    float4 g_aLightField[6] : packoffset(c77);
    float4 g_MotionBlur_AlphaRef_VelocityLimit_VelocityCutoff_BlurMagnitude : packoffset(c85);
    float4 mrgPlayableParam : packoffset(c86);
    float4 mrgDebugDistortionParam : packoffset(c87);
    float4 mrgEdgeEmissionParam : packoffset(c88);
    float4 g_ForceAlphaColor : packoffset(c89);
    row_major float4x4 g_MtxView : packoffset(c90);
    row_major float4x4 g_MtxInvView : packoffset(c94);
    row_major float4x4 g_MtxLightViewProjection : packoffset(c98);
    row_major float4x4 g_MtxVerticalLightViewProjection : packoffset(c102);
    float4 mrgVsmEpsilon : packoffset(c148);
    float4 g_DebugValue : packoffset(c149);

#ifndef GLOBALS_PS_APPEND_PARAMETERS_BEGIN
}
#endif

#define GLOBALS_PS_APPEND_PARAMETERS_END }

#endif

#endif