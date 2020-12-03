// Reference: https://gitlab.com/willp-public/redbud/-/blob/master/RedBud/Source/RedBud/Shaders/HLSL/ScreenSpaceReflections/SSLR/ConeTracing_PS.hlsl

#include "../../global.psparam.hlsl"
#include "../Deferred/Deferred.hlsl"

float4 mrgLodParam : register(c150);
float4 g_FramebufferSize : register(c151);
float4 g_StepCount_MaxRoughness_RayLength_Fade : register(c152);
float4 g_MaxSpecularExponent_Saturation_Brightness : register(c153);

sampler2D g_RLRTexCoordSampler : register(s5);

float roughnessToSpecularPower(float roughness)
{
	return exp2(g_MaxSpecularExponent_Saturation_Brightness.x * (1.0f - roughness));
}

float specularPowerToConeAngle(float specularPower)
{
	if (specularPower >= exp2(g_MaxSpecularExponent_Saturation_Brightness.x))
		return 0.0f;

	const float xi = 0.244f;
	float exponent = 1.0f / (specularPower + 1.0f);
	return acos(pow(xi, exponent));
}

float isoscelesTriangleOpposite(float adjacentLength, float coneTheta)
{
	return 2.0f * tan(coneTheta) * adjacentLength;
}

float isoscelesTriangleInRadius(float a, float h)
{
	float a2 = a * a;
	float fh2 = 4.0f * h * h;
	return (a * (sqrt(a2 + fh2) - a)) / (4.0f * h);
}

float4 coneSampleWeightedColor(float2 samplePos, float mipChannel, float gloss)
{
	float3 sampleColor = tex2Dlod(g_FramebufferSampler, float4(samplePos, 0, mipChannel)).rgb;
	return float4(sampleColor * gloss, gloss);
}

float isoscelesTriangleNextAdjacent(float adjacentLength, float incircleRadius)
{
	return adjacentLength - (incircleRadius * 2.0f);
}

float4 main(in float2 texCoord : TEXCOORD0) : COLOR
{
    float2 gBuffer2 = tex2Dlod(g_GBuffer2Sampler, float4(texCoord.xy, 0, 0));
    float2 rayCoord = tex2Dlod(g_RLRTexCoordSampler, float4(texCoord.xy, 0, 0)).xy;

    if (rayCoord.x < 0)
        return 0;

	float gloss = 1.0f - gBuffer2.y;
	float specularPower = roughnessToSpecularPower(gBuffer2.y);
	float coneTheta = specularPowerToConeAngle(specularPower) * 0.5f;
	float2 deltaP = rayCoord.xy - texCoord.xy;
	float adjacentLength = length(deltaP);
	float2 adjacentUnit = normalize(deltaP);
	float4 totalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float remainingAlpha = 1.0f;

	float maxMipLevel = (float)mrgLodParam.x - 1.0f;
	float glossMult = gloss;

	for (int i = 0; i < 14; ++i)
	{
		float oppositeLength = isoscelesTriangleOpposite(adjacentLength, coneTheta);
		float incircleSize = isoscelesTriangleInRadius(oppositeLength, adjacentLength);
		float2 samplePos = texCoord.xy + adjacentUnit * (adjacentLength - incircleSize);
		float mipChannel = clamp(log2(incircleSize * max(g_FramebufferSize.x, g_FramebufferSize.y)), 0.0f, maxMipLevel);
		float4 newColor = coneSampleWeightedColor(samplePos, mipChannel, glossMult);

		remainingAlpha -= newColor.a;

		if (remainingAlpha < 0.0f)
			newColor.rgb *= 1.0f - abs(remainingAlpha);

		totalColor += newColor;

		if (totalColor.a >= 1.0f)
			break;

		adjacentLength = isoscelesTriangleNextAdjacent(adjacentLength, incircleSize);
		glossMult *= gloss;
	}

	totalColor.rgb = lerp(dot(totalColor.rgb, float3(0.2126, 0.7152, 0.0722)), totalColor.rgb, g_MaxSpecularExponent_Saturation_Brightness.y) * g_MaxSpecularExponent_Saturation_Brightness.z;

	float factor;

	factor = saturate(rayCoord.x * g_StepCount_MaxRoughness_RayLength_Fade.w);
	factor *= saturate(rayCoord.y * g_StepCount_MaxRoughness_RayLength_Fade.w);

	factor *= saturate((1 - rayCoord.x) * g_StepCount_MaxRoughness_RayLength_Fade.w);
	factor *= saturate((1 - rayCoord.y) * g_StepCount_MaxRoughness_RayLength_Fade.w);

    return totalColor * factor;
}