#ifndef SCENE_DEFERRED_DEFERRED_INCLUDED
#define SCENE_DEFERRED_DEFERRED_INCLUDED

#include "../../global.psparam.hlsl"
#include "../Material.hlsl"

float4 g_LocalLightData[64] : register(c106);
float4 g_ShadowMapSize : register(c170);

sampler2D g_GBuffer0Sampler : register(s0);
sampler2D g_GBuffer1Sampler : register(s1);
sampler2D g_GBuffer2Sampler : register(s2);
sampler2D g_GBuffer3Sampler : register(s3);

#endif