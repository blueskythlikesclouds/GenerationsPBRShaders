#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 ChrEye1 : packoffset(c150);
float4 ChrEye2 : packoffset(c151);
float4 ChrEye3 : packoffset(c152);
float4 Blend : packoffset(c153);

GLOBALS_PS_APPEND_PARAMETERS_END

#include "../SharedPS.hlsli"

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texSpecular : register(t1);
Texture2D<float4> texCdr : register(t2);
Texture2D<float4> texReflection : register(t3);

SamplerState sampDiffuse : register(s0);
SamplerState sampSpecular : register(s1);
SamplerState sampCdr : register(s2);
SamplerState sampReflection : register(s3);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));
    params.Albedo = SrgbToLinear(diffuse.rgb);

    ConvertSpecularToParams(texSpecular.Sample(sampSpecular, UV(1)), false, params);
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input)
{
#ifdef HasSamplerCdr
    params.Cdr = texCdr.SampleLevel(sampCdr, float2(dot(params.Normal, -mrgGlobalLight_Direction.xyz) * 0.5 + 0.5, input.Color.y), 0).rgb;
#endif

    float3 eyeNormal = normalize(input.EyeNormal);
    float2 directionOffset = float2(dot(eyeNormal, float3(-1, 0, 0)), dot(eyeNormal, float3(0, 1, 0)));

    float2 diffuseAlphaOffset = -(ChrEye1.zw * directionOffset);
    float2 reflectionOffset = ChrEye3.xy + ChrEye3.zw * directionOffset;
    float2 reflectionAlphaOffset = ChrEye1.xy - float2(directionOffset.x < 0 ? ChrEye2.x : ChrEye2.y, directionOffset.y < 0 ? ChrEye2.z : ChrEye2.w) * directionOffset;

    float diffuseAlpha = texDiffuse.Sample(sampDiffuse, UV(0) + diffuseAlphaOffset).a;
    float3 reflection = texReflection.Sample(sampReflection, UV(0) + reflectionOffset).rgb;
    float reflectionAlpha = texReflection.Sample(sampReflection, UV(3) + reflectionAlphaOffset).a;

    params.Albedo = saturate(lerp((params.Albedo + reflection * texSpecular.Sample(sampSpecular, UV(1)).a) * diffuseAlpha, 1, reflectionAlpha));
    params.AmbientOcclusion = lerp(params.AmbientOcclusion, 1, reflectionAlpha);

#ifdef AddEmission
    params.Emission = params.Albedo * Blend.x * GetToneMapLuminance();
#endif
}

#include "../DefaultPS.hlsli"