#ifndef SCENE_DEFERRED_DEFERRED_INCLUDED
#define SCENE_DEFERRED_DEFERRED_INCLUDED

#include "../../global.psparam.hlsl"
#include "../Material.hlsl"
#include "../Param.hlsl"

sampler2D g_GBuffer0Sampler : register(s0);
sampler2D g_GBuffer1Sampler : register(s1);
sampler2D g_GBuffer2Sampler : register(s2);
sampler2D g_GBuffer3Sampler : register(s3);

#endif