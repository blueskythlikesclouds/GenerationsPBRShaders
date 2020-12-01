#ifndef SCENE_DECLARATIONS_HLSL_INCLUDED
#define SCENE_DECLARATIONS_HLSL_INCLUDED

struct DefaultDeclaration
{
	float3 Position : TEXCOORD0;
	float4 TexCoord0 : TEXCOORD1;
	float4 TexCoord1 : TEXCOORD2;
	float3 Normal : TEXCOORD3;
	float4 ShadowMapCoord : TEXCOORD4;
	float4 ExtraParams : TEXCOORD5;
	float4 Color : COLOR;
	float2 VPos : VPOS;
};

struct NormalDeclaration : DefaultDeclaration
{
	float3 Tangent : TEXCOORD6;
	float3 Binormal : TEXCOORD7;
};

struct EyeDeclaration : DefaultDeclaration
{
	float3 EyeNormal : TEXCOORD6;
};

float2 GetTexCoord(DefaultDeclaration input, uint index, float mrgTexcoordIndex[16])
{
#if defined(ConstTexCoord) && ConstTexCoord
	return input.TexCoord0.xy;
#endif
	float value = round(mrgTexcoordIndex[index]);

	if (value == 0.0f) return input.TexCoord0.xy;
	if (value == 1.0f) return input.TexCoord0.zw;
	if (value == 2.0f) return input.TexCoord1.xy;
	if (value == 3.0f) return input.TexCoord1.zw;

	return 0.0f;
}

#define UV(i) \
    GetTexCoord((DefaultDeclaration)input, i, mrgTexcoordIndex)

#endif