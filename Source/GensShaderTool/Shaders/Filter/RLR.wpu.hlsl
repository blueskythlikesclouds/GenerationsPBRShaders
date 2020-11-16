#include "../../global.psparam.hlsl"
#include "../Functions.hlsl"

#include "../Deferred/Deferred.hlsl"

#define g_FadeParam                   0.1

#define g_RayStepCount                32
#define g_RayLength                   50

#define g_BinarySearchStepCount       4

float4 main(in float2 vPos : TEXCOORD0, in float2 texCoord : TEXCOORD1) : COLOR
{
    float4 gBuffer2 = tex2Dlod(g_GBuffer2Sampler, float4(texCoord, 0, 0));
    float4 gBuffer3 = tex2Dlod(g_GBuffer3Sampler, float4(texCoord, 0, 0));

    float sggiRoughness = saturate((gBuffer2.y - 0.3) * 2.85714);
    if (gBuffer3.a < 0.99 || sggiRoughness > 0.8)
        return 0;

    float depth = tex2Dlod(g_DepthSampler, float4(texCoord.xy, 0, 0)).x;
    float3 position = GetPositionFromDepth(vPos, depth, g_MtxInvProjection);
    float3 dir = normalize(reflect(position, mul(gBuffer3.xyz * 2 - 1, g_MtxView).xyz));

    float4 color = 0;

    float factor = dot(dir, float3(0, 0, -1));
    if (factor > 0.0)
    {
        float3 endPosition = position + Hash(position) * gBuffer2.y + dir * g_RayLength;

        float3 begin = float3(texCoord, position.z);
        float4 end = mul(float4(endPosition, 1), g_MtxProjection);

        end.xy = end.xy / end.w * float2(0.5, -0.5) + 0.5;
        end.z = endPosition.z;

        float2 delta = end - begin;
        float2 deltaPixel = round(abs(delta) * g_ViewportSize.xy);

        int stepCount = min(g_RayStepCount, abs(max(deltaPixel.x, deltaPixel.y)));
        float stepSize = 1.0 / stepCount;

        [loop] for (int i = 0; i < stepCount; i++)
        {
            float step = (i + 0.5) * stepSize;

            float2 rayCoord = lerp(begin.xy, end.xy, step);
            float depth = (begin.z * end.z) / lerp(end.z, begin.z, step);

            float cmpDepth = tex2Dlod(g_DepthSampler, float4(rayCoord.xy, 0, 0)).x;
            cmpDepth = LinearizeDepth(cmpDepth, g_MtxInvProjection);

            if (depth < cmpDepth)
            {
                step -= stepSize;
                stepSize *= 0.5;
                step += stepSize;
                step = saturate(step);

                [loop] for (int j = 0; j < g_BinarySearchStepCount; j++)
                {
                    rayCoord = lerp(begin.xy, end.xy, step);
                    depth = (begin.z * end.z) / lerp(end.z, begin.z, step);

                    cmpDepth = tex2Dlod(g_DepthSampler, float4(rayCoord.xy, 0, 0)).x;
                    cmpDepth = LinearizeDepth(cmpDepth, g_MtxInvProjection);

                    stepSize *= 0.5;

                    if (depth < cmpDepth)
                        step -= stepSize;
                    else
                        step += stepSize;

                    step = saturate(step);
                }

                rayCoord = lerp(begin.xy, end.xy, step);

                factor *= saturate(rayCoord.x / g_FadeParam);
                factor *= saturate(rayCoord.y / g_FadeParam);

                factor *= saturate((1 - rayCoord.x) / g_FadeParam);
                factor *= saturate((1 - rayCoord.y) / g_FadeParam);

                factor *= dot(dir, mul(float4(tex2Dlod(g_GBuffer3Sampler, float4(rayCoord, 0, 0)).xyz * 2 - 1, 0), g_MtxView).xyz) < 0;

                color = tex2Dlod(g_FramebufferSampler, float4(rayCoord.xy, 0, 0)) * saturate(factor);
            }
        }
    }

    color.rgb = lerp(color.rgb, dot(color.rgb, float3(0.2126, 0.7152, 0.0722)), 0.3);
    return color;
}