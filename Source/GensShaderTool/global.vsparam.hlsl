#ifndef GLOBAL_VSPARAM_HLSL_INCLUDED
#define GLOBAL_VSPARAM_HLSL_INCLUDED
row_major float4x4 g_MtxProjection : register(c0);

row_major float4x4 g_MtxView : register(c4);

row_major float4x4 g_MtxWorld : register(c8);

row_major float4x4 g_MtxWorldIT : register(c12);

row_major float4x4 g_MtxPrevView : register(c16);

row_major float4x4 g_MtxPrevWorld : register(c20);

row_major float4x4 g_MtxLightViewProjection : register(c24);

row_major float3x4 g_MtxPalette[25] : register(c28);

row_major float3x4 g_MtxPrevPalette[25] : register(c103);

float4 g_EyePosition : register(c178);

float4 g_EyeDirection : register(c179);

float4 g_ViewportSize : register(c180);

float4 g_CameraNearFarAspect : register(c181);

float4 mrgAmbientColor : register(c182);

float4 mrgGlobalLight_Direction : register(c183);

float4 mrgGlobalLight_Diffuse : register(c184);

float4 mrgGlobalLight_Specular : register(c185);

float4 mrgGIAtlasParam : register(c186);

float4 mrgTexcoordIndex[4] : register(c187);

float4 mrgTexcoordOffset[2] : register(c191);

float4 mrgFresnelParam : register(c193);

float4 mrgMorphWeight : register(c194);

float4 mrgZOffsetRate : register(c195);

float4 g_IndexCount : register(c196);

float4 g_LightScattering_Ray_Mie_Ray2_Mie2 : register(c197);

float4 g_LightScattering_ConstG_FogDensity : register(c198);

float4 g_LightScatteringFarNearScale : register(c199);

float4 g_LightScatteringColor : register(c200);

float4 g_LightScatteringMode : register(c201);

row_major float4x4 g_MtxBillboardY : register(c202);

float4 mrgLocallightIndexArray : register(c206);

row_major float4x4 g_MtxVerticalLightViewProjection : register(c207);

float4 g_VerticalLightDirection : register(c211);

float4 g_TimeParam : register(c212);

float4 g_aLightField[6] : register(c213);

float4 g_SkyParam : register(c219);

float4 g_ViewZAlphaFade : register(c220);

float4 g_PowerGlossLevel : register(c245);

bool mrgHasBone : register(b0);

bool g_IsShadowMapEnable : register(b1);

bool g_IsLightScatteringEnable : register(b2);

#endif
