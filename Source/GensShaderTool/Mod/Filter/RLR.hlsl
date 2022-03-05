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
    float g_AccuracyThreshold;
    float g_Saturation;
    float g_Brightness;
}

bool LineIntersection(float2 a1, float2 a2, float2 b1, float2 b2, out float2 intersection)
{
    intersection = 0;

    float2 b = a2 - a1;
    float2 d = b2 - b1;
    float factor = b.x * d.y - b.y * d.x;

    if (factor == 0)
        return false;

    float2 c = b1 - a1;
    float t = (c.x * d.y - c.y * d.x) / factor;
    if (t < 0 || t > 1)
        return false;

    float u = (c.x * b.y - c.y * b.x) / factor;
    if (u < 0 || u > 1)
        return false;

    intersection = a1 + t * b;
    return true;
}

float4 main(in float4 unused : SV_POSITION, in float4 svPos : TEXCOORD0, in float4 texCoord : TEXCOORD1) : SV_TARGET
{
    ShaderParams params = LoadParams(texCoord.xy);
    if (!(params.DeferredFlags & DEFERRED_FLAGS_IBL) || params.Roughness > g_MaxRoughness)
        return 0;

    float3 position = GetPositionFromDepth(svPos.xy, g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord.xy, 0), g_MtxInvProjection);
    float3 dir = normalize(reflect(normalize(position), normalize(mul(float4(params.Normal, 0), g_MtxView).xyz)));

    float4 color = 0;

    float factor = dot(dir, float3(0, 0, -1));
    if (factor > 0.0)
    {
        float3 endPosition = position + dir * g_RayLength;

        float3 begin = float3(texCoord.xy, position.z);
        float4 end = mul(float4(endPosition, 1), g_MtxProjection);

        end.xy = end.xy / end.w * float2(0.5, -0.5) + 0.5;
        end.z = endPosition.z;

        if (end.x < 0 || end.x > 1.0 || end.y < 0 || end.y > 1.0)
        {
            float2 points[] =
            {
                float2(0, 0), float2(1, 0),
                float2(0, 0), float2(0, 1),
                float2(1, 1), float2(1, 0),
                float2(1, 1), float2(0, 1)
            };

            float2 intersectionResult = end.xy;

            [unroll] for (int i = 0; i < 4; i++)
            {
                float2 intersection;
                if (LineIntersection(points[i * 2 + 0], points[i * 2 + 1], begin.xy, end.xy, intersection))
                    intersectionResult = intersection;
            }

            float dist1 = length(begin.xy - end.xy);
            float dist2 = length(begin.xy - intersectionResult);

            end.xy = intersectionResult;
            end.z = PerspectiveCorrectLerp(begin.z, end.z, dist2 / dist1);
        }

        float2 delta = end.xy - begin.xy;
        begin.xy += normalize(delta) * g_FramebufferSize.zw;

        float2 deltaPixel = delta * g_FramebufferSize.xy;

        uint stepCount = min(g_StepCount, uint(round(sqrt(dot(deltaPixel, deltaPixel)))));
        float stepSize = 1.0f / stepCount;

        [loop] for (uint i = 0; i < stepCount; i++)
        {
            float step = (i + 0.5) * stepSize;

            float2 rayCoord = lerp(begin.xy, end.xy, step);
            float depth = PerspectiveCorrectLerp(begin.z, end.z, step);

            float fbDepth = g_DepthTexture.SampleLevel(g_PointClampSampler, rayCoord.xy, 0);
            float cmpDepth = LinearizeDepth(fbDepth, g_MtxInvProjection);

            if (fbDepth < 1 && depth < cmpDepth)
            {
                // If we hit a pixel closer to the camera than
                // the start point, skip it. It'll likely be incorrect.
                if (begin.z < cmpDepth)
                    continue;

                float tmpStepSize = stepSize;

                step -= tmpStepSize;
                tmpStepSize *= 0.5f;
                step += tmpStepSize;

                [loop] for (int j = 0; j < 4; j++)
                {
                    rayCoord = lerp(begin.xy, end.xy, step);
                    depth = PerspectiveCorrectLerp(begin.z, end.z, step);

                    cmpDepth = g_DepthTexture.SampleLevel(g_PointClampSampler, rayCoord.xy, 0);
                    cmpDepth = LinearizeDepth(cmpDepth, g_MtxInvProjection);

                    tmpStepSize *= 0.5f;

                    if (depth < cmpDepth)
                        step -= tmpStepSize;
                    else
                        step += tmpStepSize;
                }

                rayCoord = lerp(begin.xy, end.xy, step);

                // Check the accuracy of the hit position and skip as necessary.
                float3 cmpPos = GetPositionFromDepth(rayCoord * float2(2, -2) + float2(-1, 1),
                    g_DepthTexture.SampleLevel(g_PointClampSampler, rayCoord.xy, 0), g_MtxInvProjection);

                float3 d = abs(cmpPos - (position + dir * distance(cmpPos, position)));

                if (max(d.x, max(d.y, d.z)) > g_AccuracyThreshold * -cmpPos.z)
                    continue;

                factor *= saturate(rayCoord.x * g_Fade);
                factor *= saturate(rayCoord.y * g_Fade);

                factor *= saturate((1 - rayCoord.x) * g_Fade);
                factor *= saturate((1 - rayCoord.y) * g_Fade);

                color = g_GBuffer0.SampleLevel(g_LinearClampSampler, rayCoord.xy, 0) * factor;
                break;
            }
        }
    }

    float luminosity = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
    color.rgb = lerp(luminosity, color.rgb, g_Saturation) * g_Brightness;

    return color;
}