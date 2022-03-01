#ifndef SHARED_HLSLI_INCLUDED
#define SHARED_HLSLI_INCLUDED

#define UV(x) \
    GetTexCoord(input, x, mrgTexcoordIndex)

struct VertexDeclaration
{
    float4 Position : POSITION;
    float4 BlendWeight : BLENDWEIGHT;
    uint4 BlendIndices : BLENDINDICES;
    float4 Normal : NORMAL;
    float4 TexCoord0 : TEXCOORD0;
    float4 TexCoord1 : TEXCOORD1;
    float4 TexCoord2 : TEXCOORD2;
    float4 TexCoord3 : TEXCOORD3;
    float4 Tangent : TANGENT;
    float4 Binormal : BINORMAL;
    float4 Color : COLOR;
};

struct PixelDeclaration
{
    float4 SVPosition : SV_POSITION;
    float3 Position : TEXCOORD0;
    float4 TexCoord0 : TEXCOORD1;

#if !defined(ConstTexCoord)
    float4 TexCoord1 : TEXCOORD2;
#endif

    float3 Normal : TEXCOORD3;

#if defined(HasFeatureNormalMapping)
    float3 Tangent : TEXCOORD4;
    float3 Binormal : TEXCOORD5;
#endif

#if defined(HasFeatureEyeNormal)
    float3 EyeNormal : TEXCOORD6;
#endif

#if defined(IsPermutationDeferred)
    float4 ShadowMapCoord : TEXCOORD7;
    float2 ExtraParams : TEXCOORD8;
#endif

    float4 Color : COLOR;
};

struct ShaderParams
{
    float3 Albedo;
    float Alpha;
    float Reflectance;
    float Roughness;
    float AmbientOcclusion;
    float Metalness;

    float2 NormalMap;
    float3 Cdr;
    float3 Emission;

    float3 FresnelReflectance;
    float3 IndirectDiffuse;
    float3 IndirectSpecular;
    float Shadow;

    float3 Normal;
    float3 ViewDirection;
    float CosViewDirection;
    float3 RoughReflectionDirection;
    float3 SmoothReflectionDirection;
};

ShaderParams CreateShaderParams()
{
    ShaderParams params;

    params.Albedo = 1.0;
    params.Alpha = 1.0;
    params.Reflectance = 0.04;
    params.Roughness = 0.5;
    params.AmbientOcclusion = 1.0;
    params.Metalness = 0.0;
    params.NormalMap = 0.0;
    params.Cdr = 1.0;
    params.Emission = 0.0;

    params.FresnelReflectance = 0.0;
    params.IndirectDiffuse = 0.0;
    params.IndirectSpecular = 0.0;
    params.Shadow = 1.0;
    params.Normal = 0.0;
    params.ViewDirection = 0.0;
    params.CosViewDirection = 0.0;
    params.RoughReflectionDirection = 0.0;
    params.SmoothReflectionDirection = 0.0;

    return params;
}

float2 GetTexCoord(in PixelDeclaration input, uint index, float4 mrgTexcoordIndex[4])
{
#if defined(ConstTexCoord)
    return input.TexCoord0.xy;
#else

    float value = trunc(mrgTexcoordIndex[index / 4][index % 4]);

    if (value == 0.0f) return input.TexCoord0.xy;
    if (value == 1.0f) return input.TexCoord0.zw;
    if (value == 2.0f) return input.TexCoord1.xy;
    if (value == 3.0f) return input.TexCoord1.zw;

    return 0.0f;
#endif
}

void ConvertSpecularToParams(float4 specular, bool explicitMetalness, inout ShaderParams params)
{
    params.Reflectance = specular.r / 4.0;
    params.Roughness = max(0.01, 1.0 - specular.g);
    params.AmbientOcclusion = specular.b;
    params.Metalness = !explicitMetalness ? specular.r > 0.9 : specular.a;
}

void ConvertPBRFactorToParams(float4 pbrFactor, inout ShaderParams params)
{
    params.Reflectance = pbrFactor.r;
    params.Roughness = max(0.01, 1.0 - pbrFactor.g);
    params.AmbientOcclusion = 1.0;
    params.Metalness = pbrFactor.r > 0.9;
}

#endif