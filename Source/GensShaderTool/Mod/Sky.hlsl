#include "GlobalsPS.hlsli"
#include "Shared.hlsli"

struct SkyDeclaration
{
    float4 SVPosition : SV_POSITION;
    float4 TexCoord0 : TEXCOORD0;

#ifndef ConstTexCoord
    float4 TexCoord1 : TEXCOORD1;
#elif defined HasFeatureVertexColor
    float4 DUMMY : TEXCOORD1;
#endif

#ifdef HasFeatureVertexColor
    // Align to GenerationsD3D11 auto-generated shader registers
    float4 DUMMY0 : TEXCOORD2;
    float4 DUMMY1 : TEXCOORD3;
    float4 DUMMY2 : TEXCOORD4;
    float4 DUMMY3 : TEXCOORD5;
    float4 DUMMY4 : TEXCOORD6;
    float4 DUMMY5 : TEXCOORD7;
    float4 DUMMY6 : TEXCOORD8;
    float4 DUMMY7 : TEXCOORD9;
    float4 DUMMY8 : TEXCOORD10;
    float4 DUMMY9 : TEXCOORD11;
    float4 DUMMY10 : TEXCOORD12;
    float4 DUMMY11 : TEXCOORD13;
    float4 DUMMY12 : TEXCOORD14;
    float4 DUMMY13 : TEXCOORD15;

    float4 Color : COLOR0;
#endif
};

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float> texTransparency : register(t1);

SamplerState sampDiffuse : register(s0);
SamplerState sampTransparency : register(s1);

float4 main(in SkyDeclaration input) : SV_TARGET0
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

#ifdef HasFeatureVertexColor
    diffuse *= input.Color;
#endif

#ifdef IsSDR
    diffuse.rgb *= g_Diffuse.rgb;
    diffuse.a *= g_OpacityReflectionRefractionSpectype.x;
    diffuse.rgb = SrgbToLinear(diffuse.rgb);
#endif

    diffuse.rgb *= g_BackGroundScale.x;

#ifdef HasSamplerTransparency
    diffuse.a *= texTransparency.Sample(sampTransparency, UV(1)).x;
#endif

    return diffuse;
}