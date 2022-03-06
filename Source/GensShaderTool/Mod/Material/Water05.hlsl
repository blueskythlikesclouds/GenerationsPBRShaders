#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 RefractionCubemap  : packoffset(c150);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texSpecular : register(t1);
Texture2D<float4> texNormal : register(t2);
Texture2D<float4> texNormal1 : register(t3);

SamplerState sampDiffuse : register(s0);
SamplerState sampSpecular : register(s1);
SamplerState sampNormal : register(s2);
SamplerState sampNormal1 : register(s3);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    params.Alpha = input.Color.w;

    ConvertSpecularToParams(texSpecular.Sample(sampSpecular, UV(1)), false, params);

    params.NormalMap = lerp(texNormal.Sample(sampNormal, UV(2)).xy, 
        texNormal1.Sample(sampNormal1, UV(3)).xy, 1.0 - RefractionCubemap.x);
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0) + params.Normal.xy * RefractionCubemap.z);
    params.Albedo = SrgbToLinear(diffuse.rgb);

    float2 baseTexCoord = input.SVPosition.xy * g_ViewportSize.zw;
    float2 texCoord = baseTexCoord + (mul(float4(params.Normal, 0), g_MtxView).xy * RefractionCubemap.y) / (input.SVPosition.w * 10.0);

    float depth = g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord, 0);
    if (depth < input.SVPosition.z)
        texCoord = baseTexCoord;

    params.Refraction.rgb = g_FramebufferTexture.SampleLevel(g_LinearClampSampler, texCoord, 0).rgb * input.Color.rgb;
    params.Refraction.w = diffuse.a;
}

#include "../DefaultPS.hlsli"