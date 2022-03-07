#include "GlobalsVS.hlsli"
#include "LightScattering.hlsli"
#include "Shared.hlsli"

void main(in VertexDeclaration input, out PixelDeclaration output)
{
#ifdef HasFeatureNormalMapping
    float3 binormal;

    if (dot(input.Binormal.xyz, input.Binormal.xyz) == 0)
        binormal = cross(input.Normal.xyz, input.Tangent.xyz) * sign(input.Tangent.w);
    else
        binormal = input.Binormal.xyz;
#endif

#ifndef HasFeatureNoBone
    if (mrgHasBone)
    {
        float3x4 blendMatrix = g_MtxPalette[input.BlendIndices[0]] * input.BlendWeight[0];
        blendMatrix += g_MtxPalette[input.BlendIndices[1]] * input.BlendWeight[1];
        blendMatrix += g_MtxPalette[input.BlendIndices[2]] * input.BlendWeight[2];
        blendMatrix += g_MtxPalette[input.BlendIndices[3]] * (1 - input.BlendWeight[0] - input.BlendWeight[1] - input.BlendWeight[2]);

        output.Position.xyz = mul(blendMatrix, float4(input.Position.xyz, 1));
        output.Normal.xyz = mul(blendMatrix, float4(input.Normal.xyz, 0));

#ifdef HasFeatureNormalMapping
        output.Tangent.xyz = mul(blendMatrix, float4(input.Tangent.xyz, 0));
        output.Binormal.xyz = mul(blendMatrix, float4(binormal, 0));
#endif

#ifdef HasFeatureEyeNormal
        output.EyeNormal = mul(blendMatrix, float4(0, 0, 1, 0));
#endif
    }
    else
    {
#endif
        output.Position.xyz = input.Position.xyz;
        output.Normal.xyz = input.Normal.xyz;

#ifdef HasFeatureNormalMapping
        output.Tangent.xyz = input.Tangent.xyz;
        output.Binormal.xyz = binormal;
#endif

#ifdef HasFeatureEyeNormal
        output.EyeNormal = float3(0, 0, 1);
#endif
#ifndef HasFeatureNoBone
    }
#endif

    output.Position.xyz = mul(float4(output.Position.xyz, 1), g_MtxWorld).xyz;

    float4 viewPosition = mul(float4(output.Position.xyz, 1), g_MtxView);
    output.SVPosition = mul(viewPosition, g_MtxProjection);

    output.TexCoord0.xy = input.TexCoord0.xy;
    output.TexCoord0.zw = input.TexCoord1.xy;
    output.TexCoord0 += mrgTexcoordOffset[0];

#ifndef ConstTexCoord
    output.TexCoord1.xy = input.TexCoord2.xy;
    output.TexCoord1.zw = input.TexCoord3.xy;
    output.TexCoord1 += mrgTexcoordOffset[1];
#endif

    output.Normal.xyz = mul(float4(output.Normal.xyz, 0), g_MtxWorld).xyz;

#ifdef HasFeatureNormalMapping
    output.Tangent.xyz = mul(float4(output.Tangent.xyz, 0), g_MtxWorld).xyz;
    output.Binormal.xyz = mul(float4(output.Binormal.xyz, 0), g_MtxWorld).xyz;
#endif

#ifndef HasFeatureDeferred
    output.ShadowMapCoord = mul(float4(output.Position.xyz, 1), g_MtxLightViewProjection);
    output.LightScattering = ComputeLightScattering(input.Position.xyz, viewPosition.xyz);
#endif

#ifndef HasFeatureNoVertexColor
    output.Color = input.Color;
#else
    output.Color = 1.0;
#endif

#ifdef HasFeatureEyeNormal
    output.EyeNormal = mul(mul(float4(output.EyeNormal, 0), g_MtxWorld), g_MtxView).xyz;
#endif
}