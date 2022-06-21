#include "../Shared.hlsli"
#include "../GlobalsPS.hlsli"

Texture2D<float4> texDiffuse : register(t0);
SamplerState sampDiffuse : register(s0);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

    params.Albedo = SrgbToLinear(lerp(input.Color.rgb, diffuse.rgb, diffuse.a));
    params.Roughness = 1.0f;
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input) {}

#include "../DefaultPS.hlsli"