#ifndef GLOBAL_PSPARAM_HLSL_INCLUDED
#define GLOBAL_PSPARAM_HLSL_INCLUDED
row_major float4x4 g_MtxProjection : register(c0);

row_major float4x4 g_MtxInvProjection : register(c4);

float4 mrgGlobalLight_Direction : register(c10);

float4 mrgGlobalLight_Direction_View : register(c11);

float4 g_Diffuse : register(c16);

float4 g_Ambient : register(c17);

float4 g_Specular : register(c18);

float4 g_Emissive : register(c19);

float4 g_PowerGlossLevel : register(c20);

float4 g_OpacityReflectionRefractionSpectype : register(c21);

float4 g_EyePosition : register(c22);

float4 g_EyeDirection : register(c23);

float4 g_ViewportSize : register(c24);

float4 g_CameraNearFarAspect : register(c25);

float4 mrgTexcoordIndex[4] : register(c26);

float4 mrgAmbientColor : register(c30);

float4 mrgGroundColor : register(c31);

float4 mrgSkyColor : register(c32);

float4 mrgPowerGlossLevel : register(c33);

float4 mrgEmissionPower : register(c34);

float4 mrgGlobalLight_Diffuse : register(c36);

float4 mrgGlobalLight_Specular : register(c37);

float4 mrgLocalLight0_Position : register(c38);

float4 mrgLocalLight0_Color : register(c39);

float4 mrgLocalLight0_Range : register(c40);

float4 mrgLocalLight0_Attribute : register(c41);

float4 mrgLocalLight1_Position : register(c42);

float4 mrgLocalLight1_Color : register(c43);

float4 mrgLocalLight1_Range : register(c44);

float4 mrgLocalLight1_Attribute : register(c45);

float4 mrgLocalLight2_Position : register(c46);

float4 mrgLocalLight2_Color : register(c47);

float4 mrgLocalLight2_Range : register(c48);

float4 mrgLocalLight2_Attribute : register(c49);

float4 mrgLocalLight3_Position : register(c50);

float4 mrgLocalLight3_Color : register(c51);

float4 mrgLocalLight3_Range : register(c52);

float4 mrgLocalLight3_Attribute : register(c53);

float4 mrgLocalLight4_Position : register(c54);

float4 mrgLocalLight4_Color : register(c55);

float4 mrgLocalLight4_Range : register(c56);

float4 mrgLocalLight4_Attribute : register(c57);

float4 mrgEyeLight_Diffuse : register(c58);

float4 mrgEyeLight_Specular : register(c59);

float4 mrgEyeLight_Range : register(c60);

float4 mrgEyeLight_Attribute : register(c61);

float4 mrgFresnelParam : register(c62);

float4 mrgLuminanceRange : register(c63);

float4 mrgInShadowScale : register(c64);

float4 g_ShadowMapParams : register(c65);

float4 mrgColourCompressFactor : register(c66);

float4 g_BackGroundScale : register(c67);

float4 g_TimeParam : register(c68);

float4 g_GIModeParam : register(c69);

float4 g_GI0Scale : register(c70);

float4 g_GI1Scale : register(c71);

float4 g_LightScattering_Ray_Mie_Ray2_Mie2 : register(c72);

float4 g_LightScattering_ConstG_FogDensity : register(c73);

float4 g_LightScatteringFarNearScale : register(c74);

float4 g_LightScatteringColor : register(c75);

float4 g_LightScatteringMode : register(c76);

float4 g_aLightField[6] : register(c77);

float4 g_MotionBlur_AlphaRef_VelocityLimit_VelocityCutoff_BlurMagnitude : register(c85);

float4 mrgPlayableParam : register(c86);

float4 mrgDebugDistortionParam : register(c87);

float4 mrgEdgeEmissionParam : register(c88);

float4 g_ForceAlphaColor : register(c89);

row_major float4x4 g_MtxView : register(c90);

row_major float4x4 g_MtxInvView : register(c94);

row_major float4x4 g_MtxLightViewProjection : register(c98);

row_major float4x4 g_MtxVerticalLightViewProjection : register(c102);

float4 mrgVsmEpsilon : register(c148);

float4 g_DebugValue : register(c149);

bool mrgIsEnableHemisphere : register(b0);

bool g_IsShadowMapEnable : register(b1);

bool g_IsAlphaDepthBlur : register(b2);

bool g_IsGIEnabled : register(b3);

bool g_IsLightScatteringEnable : register(b4);

bool g_IsSoftParticle : register(b5);

sampler g_VerticalShadowMapSampler : register(s7);

sampler g_INDEXEDLIGHTMAPSampler : register(s8);

sampler g_TerrainDiffusemapMaskSampler : register(s9);

sampler g_GISampler : register(s10);

sampler g_FramebufferSampler : register(s11);

sampler g_DepthSampler : register(s12);

sampler g_ShadowMapSampler : register(s13);

sampler g_ShadowMapJitterSampler : register(s14);

sampler g_ReflectionMapSampler : register(s14);

sampler g_ReflectionMap2Sampler : register(s15);

#endif
