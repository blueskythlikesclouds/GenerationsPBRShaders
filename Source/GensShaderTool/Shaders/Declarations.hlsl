#ifndef SCENE_DECLARATIONS_HLSL_INCLUDED
#define SCENE_DECLARATIONS_HLSL_INCLUDED

struct DefaultDeclaration
{
	float3 Position : TEXCOORD0;
	float4 TexCoord0 : TEXCOORD1;
	float4 TexCoord1 : TEXCOORD2;
	float3 Normal : TEXCOORD3;
	float4 ShadowMapCoord : TEXCOORD4;
	float4 Color : COLOR;
};

struct NormalDeclaration : DefaultDeclaration
{
	float3 Tangent : TEXCOORD5;
	float3 Binormal : TEXCOORD6;
};

struct EyeDeclaration : DefaultDeclaration
{
	float3 EyeNormal : TEXCOORD5;
};

struct WaterDeclaration : NormalDeclaration
{
	float4 SvPosition : TEXCOORD7;
};

float2 GetTexCoord(DefaultDeclaration input, uint index, float mrgTexcoordIndex[16])
{
#if defined(ConstTexCoord) && ConstTexCoord
	return input.TexCoord0.xy;
#endif

	switch (abs(round(mrgTexcoordIndex[index])))
	{
	case 0:
		return input.TexCoord0.xy;

	case 1:
		return input.TexCoord0.zw;

	case 2:
		return input.TexCoord1.xy;

	case 3:
		return input.TexCoord1.zw;
	}

	return input.TexCoord0.xy;
}

#define UV(i) \
    GetTexCoord((DefaultDeclaration)input, i, mrgTexcoordIndex)

#endif