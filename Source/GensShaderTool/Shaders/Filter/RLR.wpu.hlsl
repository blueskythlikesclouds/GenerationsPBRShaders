#include "../../global.psparam.hlsl"
#include "../Functions.hlsl"

#include "../Deferred/Deferred.hlsl"

float4 g_FramebufferSize : register(c150);
float4 g_StepCount_MaxRoughness_RayLength_Fade : register(c151);
float4 g_AccuracyThreshold_Saturation_Brightness : register(c152);

float4 main(in float2 vPos : TEXCOORD0, in float2 texCoord : TEXCOORD1) : COLOR
{
    float4 gBuffer2 = tex2Dlod(g_GBuffer2Sampler, float4(texCoord, 0, 0));
    float4 gBuffer3 = tex2Dlod(g_GBuffer3Sampler, float4(texCoord, 0, 0));

    uint type = UnpackPrimitiveType(gBuffer3.w);

    if (type == PRIMITIVE_TYPE_RAW || type == PRIMITIVE_TYPE_EMISSION || gBuffer2.y > g_StepCount_MaxRoughness_RayLength_Fade.y)
        return 0;

    float depth = tex2Dlod(g_DepthSampler, float4(texCoord.xy, 0, 0)).x;
    float3 position = GetPositionFromDepth(vPos, depth, g_MtxInvProjection);
    float3 dir = normalize(ComputeRoughReflectionDirection(gBuffer2.y, normalize(mul(float4(gBuffer3.xyz * 2 - 1, 0), g_MtxView).xyz), normalize(-position)));

    float4 color = 0;

    float factor = dot(dir, float3(0, 0, -1));
    if (factor > 0.0)
    {
        float3 endPosition = position + dir * g_StepCount_MaxRoughness_RayLength_Fade.z;

        float3 begin = float3(texCoord, position.z);
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
                if (LineIntersection(points[i * 2 + 0], points[i * 2 + 1], begin, end, intersection))
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
        
        int stepCount = min(g_StepCount_MaxRoughness_RayLength_Fade.x, round(sqrt(dot(deltaPixel, deltaPixel))));
        float stepSize = 1.0f / stepCount;

        [loop] for (int i = 0; i < stepCount; i++)
        {
            float step = (i + 0.5) * stepSize;

            float2 rayCoord = lerp(begin.xy, end.xy, step);
            depth = PerspectiveCorrectLerp(begin.z, end.z, step);

            float fbDepth = tex2Dlod(g_DepthSampler, float4(rayCoord.xy, 0, 0)).x;
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

                    cmpDepth = tex2Dlod(g_DepthSampler, float4(rayCoord.xy, 0, 0)).x;
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
                    tex2Dlod(g_DepthSampler, float4(rayCoord.xy, 0, 0)).x, g_MtxInvProjection);

                float3 d = abs(cmpPos - (position + dir * distance(cmpPos, position)));

                if (max(d.x, max(d.y, d.z)) > g_AccuracyThreshold_Saturation_Brightness.x * -cmpPos.z)
                    continue;

                factor *= saturate(rayCoord.x * g_StepCount_MaxRoughness_RayLength_Fade.w);
                factor *= saturate(rayCoord.y * g_StepCount_MaxRoughness_RayLength_Fade.w);

                factor *= saturate((1 - rayCoord.x) * g_StepCount_MaxRoughness_RayLength_Fade.w);
                factor *= saturate((1 - rayCoord.y) * g_StepCount_MaxRoughness_RayLength_Fade.w);

                color = tex2Dlod(g_FramebufferSampler, float4(rayCoord.xy, 0, 0)) * factor;
                break;
            }
        }
    }

    float luminosity = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
    color.rgb = lerp(luminosity, color.rgb, g_AccuracyThreshold_Saturation_Brightness.y) * g_AccuracyThreshold_Saturation_Brightness.z;

    return color;
}