#ifndef HEIGHT_MAP_SHARED_HLSLI_INCLUDED
#define HEIGHT_MAP_SHARED_HLSLI_INCLUDED

struct HeightMapVSDeclaration
{
    float3 Position : POSITION;
};

struct HeightMapPSDeclaration
{
    float4 SVPosition : SV_POSITION;
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

#endif