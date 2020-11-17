#include "../global.vsparam.hlsl"

#include "Material/Default.hlsl"

bool g_IsUseDeferred : register(b6);

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

    [branch] if (input.Tangent.w == 0)
        binormal = input.Binormal.xyz;
    else
        binormal = cross(input.Normal.xyz, input.Tangent.xyz) * sign(input.Tangent.w);
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

    svPosition = mul(mul(float4(output.Position.xyz, 1), g_MtxView), g_MtxProjection);
    svPosition.xy += g_ViewportSize.zw * float2(-1, 1) * svPosition.w;

#if defined(IsWater2) && IsWater2
    output.SvPosition.xy = svPosition.xy * float2(0.5, -0.5) + svPosition.w * 0.5;
    output.SvPosition.zw = svPosition.zw;
#endif

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

    if (!g_IsUseDeferred)
        output.ShadowMapCoord = mul(float4(output.Position.xyz, 1), g_MtxLightViewProjection);

#if 0
#if !IsXbox360
    output.extraParams.z = 1 - saturate(dot(float4(output.normal.xyz, 1), g_VerticalLightDirection));
#else
    output.extraParams.z = 0;
#endif

    // Light scattering code from Vanilla vertex shaders
    {
        float4 r0 = 0, r1 = 0, r2 = 0, r3 = 0, r4 = 0;

        r1.xyz = g_EyePosition.xyzw + -output.Position.xyz;
        r2.xyz = normalize(r1.xyz);
        r1.x = dot(-mrgGlobalLight_Direction.xyzw, r2.xyz);
        r1.y = g_LightScattering_ConstG_FogDensity.z * r1.x + g_LightScattering_ConstG_FogDensity.y;
        r1.x = r1.x * r1.x + 1;
        r2.x = pow(abs(r1.y), 1.5);
        r1.y = 1.0 / r2.x;
        r1.y = g_LightScattering_ConstG_FogDensity.x * r1.y;
        r1.y = g_LightScattering_Ray_Mie_Ray2_Mie2.w * r1.y;
        r1.x = g_LightScattering_Ray_Mie_Ray2_Mie2.z * r1.x + r1.y;
        r1.y = g_LightScattering_Ray_Mie_Ray2_Mie2.x + g_LightScattering_Ray_Mie_Ray2_Mie2.y;
        r1.z = 1.0 / r1.y;
        r1.x = r1.x * r1.z;
        r2 = mul(float4(output.Position.xyz, 1), g_MtxView);
        r0.x = -g_LightScatteringFarNearScale.y + -r2.z;
        r0.x = saturate(g_LightScatteringFarNearScale.x * r0.x);
        r0.x = g_LightScatteringFarNearScale.z * r0.x;
        r0.x = -r1.y * r0.x;
        r0.x = LOGE2 * r0.x;
        r0.x = exp2(r0.x);
        r0.y = 1 + -r0.x;
        output.extraParams.x = r0.x;
        r0.x = r1.x * r0.y;
        output.extraParams.y = g_LightScatteringFarNearScale.w * r0.x;
    }
#endif

    output.Color = input.Color;

#if defined(IsEye2) && IsEye2
    output.EyeNormal = mul(mul(float4(output.EyeNormal, 0), g_MtxWorld), g_MtxView).xyz;
#endif
}