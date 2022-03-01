#include "GlobalsVS.hlsli"
#include "Shared.hlsli"

void main(in VertexDeclaration input, out PixelDeclaration output)
{
#if defined(HasFeatureNormalMapping)
    float3 binormal;

    [branch] if (dot(input.Binormal.xyz, input.Binormal.xyz) == 0)
        binormal = cross(input.Normal.xyz, input.Tangent.xyz) * sign(input.Tangent.w);
    else
        binormal = input.Binormal.xyz;
#endif

#if defined(HasFeatureNoBone)
    bool hasBone = false;
#else
    bool hasBone = mrgHasBone;
#endif

    if (hasBone)
    {
        float3x4 blendMatrix = g_MtxPalette[input.BlendIndices[0]] * input.BlendWeight[0];
        blendMatrix += g_MtxPalette[input.BlendIndices[1]] * input.BlendWeight[1];
        blendMatrix += g_MtxPalette[input.BlendIndices[2]] * input.BlendWeight[2];
        blendMatrix += g_MtxPalette[input.BlendIndices[3]] * (1 - input.BlendWeight[0] - input.BlendWeight[1] - input.BlendWeight[2]);

        output.Position.xyz = mul(blendMatrix, float4(input.Position.xyz, 1));
        output.Normal.xyz = mul(blendMatrix, float4(input.Normal.xyz, 0));

#if defined(HasFeatureNormalMapping)
        output.Tangent.xyz = mul(blendMatrix, float4(input.Tangent.xyz, 0));
        output.Binormal.xyz = mul(blendMatrix, float4(binormal, 0));
#endif

#if defined(HasFeatureEyeNormal)
        output.EyeNormal = mul(blendMatrix, float4(0, 0, 1, 0));
#endif
    }
    else
    {
        output.Position.xyz = input.Position.xyz;
        output.Normal.xyz = input.Normal.xyz;

#if defined(HasFeatureNormalMapping)
        output.Tangent.xyz = input.Tangent.xyz;
        output.Binormal.xyz = binormal;
#endif

#if defined(HasFeatureEyeNormal)
        output.EyeNormal = float3(0, 0, 1);
#endif
    }

    output.Position.xyz = mul(float4(output.Position.xyz, 1), g_MtxWorld).xyz;

    float4 viewPosition = mul(float4(output.Position.xyz, 1), g_MtxView);
    output.SVPosition = mul(viewPosition, g_MtxProjection);

    output.TexCoord0.xy = input.TexCoord0.xy;
    output.TexCoord0.zw = input.TexCoord1.xy;
    output.TexCoord0 += mrgTexcoordOffset[0];

#if !defined(ConstTexCoord)
    output.TexCoord1.xy = input.TexCoord2.xy;
    output.TexCoord1.zw = input.TexCoord3.xy;
    output.TexCoord1 += mrgTexcoordOffset[1];
#endif

    output.Normal.xyz = mul(float4(output.Normal.xyz, 0), g_MtxWorld).xyz;

#if defined(HasFeatureNormalMapping)
    output.Tangent.xyz = mul(float4(output.Tangent.xyz, 0), g_MtxWorld).xyz;
    output.Binormal.xyz = mul(float4(output.Binormal.xyz, 0), g_MtxWorld).xyz;
#endif

#if defined(IsPermutationDeferred)
    output.ShadowMapCoord = mul(float4(output.Position.xyz, 1), g_MtxLightViewProjection);
    //output.ExtraParams.xy = ComputeLightScattering(input.Position, viewPosition.xyz);
#endif

#if !defined(HasFeatureNoVertexColor)
    output.Color = input.Color;
#endif

#if defined(HasFeatureEyeNormal)
    output.EyeNormal = mul(mul(float4(output.EyeNormal, 0), g_MtxWorld), g_MtxView).xyz;
#endif
}