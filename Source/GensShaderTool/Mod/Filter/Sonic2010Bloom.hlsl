#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 g_MiddleGray_Scale_LuminanceLow_LuminanceHigh : packoffset(c150);
float4 g_BloomStar_Param1 : packoffset(c151);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDif0 : register(t0);
Texture2D<float4> texDif1 : register(t1);

SamplerState sampDif0 : register(s0);
SamplerState sampDif1 : register(s1);

float3 GetToneMappedColor(float2 v0)
{
	float4 c0;
	float4 c1;

	c0 = float4(0.212500006, 0.715399981, 0.0720999986, 0.5);
	c1 = float4(0.00100000005, 1, 0, 0);

	float4 r0 = float4(0, 0, 0, 0);
	float4 r1 = float4(0, 0, 0, 0);

	r0 = texDif1.Load(int3(0, 0, 0));
	r0.x = r0.x + c1.x;
	r0.x = rcp(r0.x);
	r1 = texDif0.Sample(sampDif0, v0);
	r0.y = dot(r1.xyz, c0.xyz);
	r0.z = r0.y * g_MiddleGray_Scale_LuminanceLow_LuminanceHigh.x;
	r0.y = rcp(r0.y);
	r0.w = r0.z * r0.x + c1.y;
	r0.x = r0.x * r0.z;
	r0.z = rcp(r0.w);
	r0.x = r0.z * r0.x;
	r0.x = r0.y * r0.x;	
	return r0.xxx * r1.xyz;
}

float4 main(in float4 unused : SV_POSITION, in float4 texCoord : TEXCOORD0) : SV_TARGET
{
	return float4(saturate(saturate(GetToneMappedColor(texCoord.xy) * 0.5) / (g_BloomStar_Param1.x * 0.5) - g_BloomStar_Param1.x), 1);
}