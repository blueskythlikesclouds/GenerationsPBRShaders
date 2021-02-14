#include "../global.vsparam.hlsl"

#include "Material/Default.hlsl"

struct Input
{
    float4 Position : POSITION;
    float4 BlendWeight : BLENDWEIGHT;
    float4 BlendIndices : BLENDINDICES;
    float4 Normal : NORMAL;
    float4 TexCoord0 : TEXCOORD0;
    float4 TexCoord1 : TEXCOORD1;
    float4 TexCoord2 : TEXCOORD2;
    float4 TexCoord3 : TEXCOORD3;
    float4 Tangent : TANGENT;
    float4 Binormal : BINORMAL;
    float4 Color : COLOR;
};

void main(in Input input, out DECLARATION_TYPE output, out float4 svPosition : POSITION)
{
#if !defined(IsBoneless) || !IsBoneless
    bool hasBone = mrgHasBone;
#else
    bool hasBone = false;
#endif

#if (defined(IsDefault2Normal) && IsDefault2Normal) || (defined(IsWater2) && IsWater2)
    float3 binormal;

    [branch] if (dot(input.Binormal.xyz, input.Binormal.xyz) == 0)
        binormal = cross(input.Normal.xyz, input.Tangent.xyz) * sign(input.Tangent.w);
    else
        binormal = input.Binormal.xyz;
#endif

    if (hasBone)
    {
        float3x4 blendMatrix = g_MtxPalette[input.BlendIndices[0]] * input.BlendWeight[0];
        blendMatrix += g_MtxPalette[input.BlendIndices[1]] * input.BlendWeight[1];
        blendMatrix += g_MtxPalette[input.BlendIndices[2]] * input.BlendWeight[2];
        blendMatrix += g_MtxPalette[input.BlendIndices[3]] * (1 - input.BlendWeight[0] - input.BlendWeight[1] - input.BlendWeight[2]);

        output.Position.xyz = mul(blendMatrix, float4(input.Position.xyz, 1));
        output.Normal.xyz = mul(blendMatrix, float4(input.Normal.xyz, 0));

#if (defined(IsDefault2Normal) && IsDefault2Normal) || (defined(IsWater2) && IsWater2)
        output.Tangent.xyz = mul(blendMatrix, float4(input.Tangent.xyz, 0));
        output.Binormal.xyz = mul(blendMatrix, float4(binormal, 0));
#endif

#if defined(IsEye2) && IsEye2
        output.EyeNormal = mul(blendMatrix, float4(0, 0, 1, 0));
#endif
    }
    else
    {
        output.Position.xyz = input.Position.xyz;
        output.Normal.xyz = input.Normal.xyz;

#if (defined(IsDefault2Normal) && IsDefault2Normal) || (defined(IsWater2) && IsWater2)
        output.Tangent.xyz = input.Tangent.xyz;
        output.Binormal.xyz = binormal;
#endif

#if defined(IsEye2) && IsEye2
        output.EyeNormal = float3(0, 0, 1);
#endif
    }

    output.Position.xyz = mul(float4(output.Position.xyz, 1), g_MtxWorld).xyz;

    float4 viewPosition = mul(float4(output.Position.xyz, 1), g_MtxView);
    svPosition = mul(viewPosition, g_MtxProjection);
    svPosition.xy += g_ViewportSize.zw * float2(-1, 1) * svPosition.w;

    output.ExtraParams.zw = svPosition.zw;

    output.TexCoord0.xy = input.TexCoord0.xy;
    output.TexCoord0.zw = input.TexCoord1.xy;
    output.TexCoord1.xy = input.TexCoord2.xy;
    output.TexCoord1.zw = input.TexCoord3.xy;

#if !defined(ConstTexCoord) || !ConstTexCoord
    output.TexCoord0 += mrgTexcoordOffset[0];
    output.TexCoord1 += mrgTexcoordOffset[1];
#endif

    output.Normal.xyz = mul(float4(output.Normal.xyz, 0), g_MtxWorld).xyz;

#if (defined(IsDefault2Normal) && IsDefault2Normal) || (defined(IsWater2) && IsWater2)
    output.Tangent.xyz = mul(float4(output.Tangent.xyz, 0), g_MtxWorld).xyz;
    output.Binormal.xyz = mul(float4(output.Binormal.xyz, 0), g_MtxWorld).xyz;
#endif

    if (!mrgIsUseDeferred)
    {
        output.ShadowMapCoord = mul(float4(output.Position.xyz, 1), g_MtxLightViewProjection);
        output.ExtraParams.xy = ComputeLightScattering(input.Position, viewPosition.xyz);
    }

    output.Color = input.Color;

#if defined(IsEye2) && IsEye2
    output.EyeNormal = mul(mul(float4(output.EyeNormal, 0), g_MtxWorld), g_MtxView).xyz;
#endif
}