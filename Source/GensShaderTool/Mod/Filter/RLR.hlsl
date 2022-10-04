#include "../GlobalsPS.hlsli"
#include "../Shared.hlsli"
#include "../SharedPS.hlsli"

cbuffer cbFilter : register(b5)
{
    float4 g_FramebufferSize;
    uint g_StepCount;
    float g_MaxRoughness;
    float g_RayLength;
    float g_Fade;
}

bool LineIntersection(float2 a1, float2 a2, float2 b1, float2 b2, out float2 intersection)
{
    intersection = 0.0;

    float2 b = a2 - a1;
    float2 d = b2 - b1;
    float factor = b.x * d.y - b.y * d.x;

    if (factor == 0.0)
        return false;

    float2 c = b1 - a1;
    float t = (c.x * d.y - c.y * d.x) / factor;
    if (t < 0.0 || t > 1.0)
        return false;

    float u = (c.x * b.y - c.y * b.x) / factor;
    if (u < 0.0 || u > 1.0)
        return false;

    intersection = a1 + t * b;
    return true;
}

float4 main(in float4 unused : SV_POSITION, in float4 svPos : TEXCOORD0, in float4 texCoord : TEXCOORD1) : SV_TARGET
{
    ShaderParams params = LoadParams(texCoord.xy);
    if (!(params.DeferredFlags & DEFERRED_FLAGS_IBL) || params.Roughness > g_MaxRoughness)
        return 0.0;

    float3 position = GetPositionFromDepth(svPos.xy, g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord.xy, 0), g_MtxInvProjection);
    float3 direction = normalize(reflect(normalize(position), normalize(mul(float4(params.Normal, 0.0), g_MtxView).xyz)));

    float4 color = 0.0;
    float factor = dot(direction, float3(0.0, 0.0, -1.0));

    if (factor > 0.0)
    {
        float3 endPosition = position + direction * g_RayLength;

        float3 startRayCoord = float3(texCoord.xy, position.z);
        float4 endRayCoord = mul(float4(endPosition, 1.0), g_MtxProjection);

        endRayCoord.xy = endRayCoord.xy / endRayCoord.w * float2(0.5, -0.5) + 0.5;
        endRayCoord.z = endPosition.z;

        if (any(endRayCoord.xy < 0.0) || any(endRayCoord.xy >= 1.0))
        {
            float2 points[] =
            {
                float2(0.0, 0.0), float2(1.0, 0.0),
                float2(0.0, 0.0), float2(0.0, 1.0),
                float2(1.0, 1.0), float2(1.0, 0.0),
                float2(1.0, 1.0), float2(0.0, 1.0)
            };

            float2 intersectionResult = endRayCoord.xy;

            [unroll] for (int i = 0; i < 4; i++)
            {
                float2 intersection;
                if (LineIntersection(points[i * 2 + 0], points[i * 2 + 1], startRayCoord.xy, endRayCoord.xy, intersection))
                    intersectionResult = intersection;
            }

            float dist1 = length(startRayCoord.xy - endRayCoord.xy);
            float dist2 = length(startRayCoord.xy - intersectionResult);

            endRayCoord.xy = intersectionResult;
            endRayCoord.z = PerspectiveCorrectLerp(startRayCoord.z, endRayCoord.z, dist2 / dist1);
        }

        float2 delta = endRayCoord.xy - startRayCoord.xy;
        startRayCoord.xy += sign(delta) * g_FramebufferSize.zw;
        float2 deltaPixel = delta * g_FramebufferSize.xy;

        uint stepCount = min(g_StepCount, uint(round(sqrt(dot(deltaPixel, deltaPixel)))));
        float stepSize = 1.0 / stepCount;

        [loop] for (uint i = 0; i < stepCount; i++)
        {
            float step = (i + 0.5) * stepSize;

            float2 rayCoord = lerp(startRayCoord.xy, endRayCoord.xy, step);
            float linearRayDepth = PerspectiveCorrectLerp(startRayCoord.z, endRayCoord.z, step);

            float projectedDepth = g_DepthTexture.SampleLevel(g_PointClampSampler, rayCoord.xy, 0);
            float linearDepth = LinearizeDepth(projectedDepth, g_MtxInvProjection);

            if (projectedDepth < 1.0 && (startRayCoord.z >= linearDepth && linearDepth >= linearRayDepth))
            {
                step -= stepSize;
                float curStepSize = stepSize / 2.0f;
                step += curStepSize;

                uint j;
                [unroll] for (j = 0; j < 4; j++)
                {
                    rayCoord = lerp(startRayCoord.xy, endRayCoord.xy, step);
                    linearRayDepth = PerspectiveCorrectLerp(startRayCoord.z, endRayCoord.z, step);

                    projectedDepth = g_DepthTexture.SampleLevel(g_PointClampSampler, rayCoord.xy, 0);
                    linearDepth = LinearizeDepth(projectedDepth, g_MtxInvProjection);

                    curStepSize /= 2.0f;

                    if (linearDepth >= linearRayDepth)
                        step -= curStepSize;
                    else
                        step += curStepSize;
                }

                rayCoord = lerp(startRayCoord.xy, endRayCoord.xy, step);
                linearRayDepth = PerspectiveCorrectLerp(startRayCoord.z, endRayCoord.z, step);

                float4 linearDepthVec = g_DepthTexture.GatherRed(g_LinearClampSampler, rayCoord.xy, 0);
                [unroll] for (j = 0; j < 4; j++)
                    linearDepthVec[j] = LinearizeDepth(linearDepthVec[j], g_MtxInvProjection);

                float accuracyThreshold = 0.001 * max(1.0, linearRayDepth * linearRayDepth);

                if (any(abs(linearDepthVec - linearRayDepth) > accuracyThreshold))
                    continue;

                factor *= saturate(rayCoord.x * g_Fade);
                factor *= saturate(rayCoord.y * g_Fade);

                factor *= saturate((1.0 - rayCoord.x) * g_Fade);
                factor *= saturate((1.0 - rayCoord.y) * g_Fade);

                color = g_GBuffer0.SampleLevel(g_LinearClampSampler, rayCoord.xy, 0) * factor;
                break;
            }   
        }
    }

    return color;
}