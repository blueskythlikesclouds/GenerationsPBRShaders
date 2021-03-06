#ifndef SCENE_FUNCTIONS_HLSL_INCLUDED
#define SCENE_FUNCTIONS_HLSL_INCLUDED

#include "Filter.hlsl"
#include "Material.hlsl"

#define PI       3.14159265358979323846f
#define LOG2E    1.44269504088896340736f

#define GAMMA    float4(2.2, 2.2, 2.2, 1.0)

float3 FresnelSchlick(float3 F0, float cosTheta)
{
    float p = (-5.55473 * cosTheta - 6.98316) * cosTheta;
    return F0 + (1.0 - F0) * exp2(p);
}

float NdfGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * alphaSq - cosLh) * cosLh + 1;
    return alphaSq / (PI * denom * denom);
}

float VisSchlick(float roughness, float cosLo, float cosLi)
{
    float r = roughness + 1;
    float k = (r * r) / 8;
    float schlickV = cosLo * (1 - k) + k;
    float schlickL = cosLi * (1 - k) + k;
    return 0.25 / (schlickV * schlickL);
}

float2 ApproxEnvBRDF(float cosLo, float roughness)
{
    float4 c0 = float4(-1, -0.0275, -0.572, 0.022);
    float4 c1 = float4(1, 0.0425, 1.04, -0.04);
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * cosLo)) * r.x + r.y;
    return float2(-1.04, 1.04) * a004 + r.zw;
}

void ComputeDirectLightingRaw(Material material, float3 lightDirection, float3 lightColor, out float3 diffuseBRDF, out float3 specularBRDF)
{
    float3 halfwayDirection = normalize(material.ViewDirection + lightDirection);

    float cosLightDirection = saturate(dot(lightDirection, material.Normal));
    float cosHalfwayDirection = saturate(dot(halfwayDirection, material.Normal));

    float3 F = FresnelSchlick(material.F0, saturate(dot(halfwayDirection, material.ViewDirection)));
    float D = NdfGGX(cosHalfwayDirection, material.Roughness);
    float Vis = VisSchlick(material.Roughness, material.CosViewDirection, cosLightDirection);

    float3 kd = lerp(1 - F, 0, material.Metalness);

    diffuseBRDF = kd * (material.Albedo / PI) * lightColor;
    specularBRDF = (D * Vis) * F * lightColor;
}

float3 ComputeDirectLightingRaw(Material material, float3 lightDirection, float3 lightColor)
{
    float3 diffuseBRDF, specularBRDF;
    ComputeDirectLightingRaw(material, lightDirection, lightColor, diffuseBRDF, specularBRDF);

    return diffuseBRDF + specularBRDF;
}

void ComputeDirectLighting(Material material, float3 lightDirection, float3 lightColor, out float3 diffuseBRDF, out float3 specularBRDF)
{
    ComputeDirectLightingRaw(material, lightDirection, lightColor, diffuseBRDF, specularBRDF);

    float cosTheta = saturate(dot(lightDirection, material.Normal));

    diffuseBRDF *= cosTheta;
    specularBRDF *= cosTheta;
}

float3 ComputeDirectLighting(Material material, float3 lightDirection, float3 lightColor)
{
    return ComputeDirectLightingRaw(material, lightDirection, lightColor) * saturate(dot(lightDirection, material.Normal));
}

float3 ComputeLocalLight(float3 position, Material material, float3 lightPosition, float3 lightColor, float4 lightRange)
{
    float3 delta = lightPosition - position;
    float distance = length(delta);
    float3 direction = delta / distance;

    float attenuation = 1.0f / (lightRange.y + lightRange.z * distance + lightRange.w * distance * distance);

    // Apply cutoff
    const float cutoff = 0.0002f;
    attenuation = saturate((attenuation - cutoff) / (1 - cutoff));

    [branch] if (attenuation > 0)
        return ComputeDirectLighting(material, direction, lightColor) * attenuation;

    return 0;
}

float3 ComputeIndirectLighting(Material material, float2 specularBRDF)
{
    float3 kd = lerp(1 - material.F0, 0, material.Metalness);

    float3 diffuseIBL = (kd * material.Albedo) * material.IndirectDiffuse;

    float3 specularIBL = (material.F0 * specularBRDF.x + specularBRDF.y) * material.IndirectSpecular;

    return (diffuseIBL + specularIBL) * material.AmbientOcclusion;
}

float3 ComputeIndirectLighting(Material material)
{
    return ComputeIndirectLighting(material, ApproxEnvBRDF(material.CosViewDirection, material.Roughness));
}

float3 ComputeIndirectLighting(Material material, sampler2D envBRDF)
{
    return ComputeIndirectLighting(material, tex2Dlod(envBRDF, float4(material.CosViewDirection, material.Roughness, 0, 0)).xy);
}

float ComputeShadow(sampler shadowTex, float4 shadowMapCoord, float2 texSize, float esmFactor)
{
    if (abs(shadowMapCoord.x) > shadowMapCoord.w || 
        abs(shadowMapCoord.y) > shadowMapCoord.w || 
        abs(shadowMapCoord.z) > shadowMapCoord.w)
        return 1.0;

    float3 shadowPos = shadowMapCoord.xyz / shadowMapCoord.w;
    shadowPos.xy = shadowPos.xy * float2(0.5, -0.5) + 0.5;

    float sum = 0;
    {
        float2 position = shadowPos.xy * texSize.x;

        float2 center;
        center.x = floor(position.x + 0.5);
        center.y = floor(position.y + 0.5);

        float s = (position.x + 0.5 - center.x);
        float t = (position.y + 0.5 - center.y);

        center = (center - 0.5) * texSize.y;

        float uw0 = (3 - 2 * s);
        float uw1 = (1 + 2 * s);
        float vw0 = (3 - 2 * t);
        float vw1 = (1 + 2 * t);

        float4 offsets;

        offsets.x = (2 - s) / uw0 - 1;
        offsets.y = s / uw1 + 1;
        offsets.z = (2 - t) / vw0 - 1;
        offsets.w = t / vw1 + 1;

        offsets *= texSize.y;

        sum += uw0 * vw0 * texESM(shadowTex, texSize, center + offsets.xz, shadowPos.z, esmFactor);
        sum += uw1 * vw0 * texESM(shadowTex, texSize, center + offsets.yz, shadowPos.z, esmFactor);
        sum += uw0 * vw1 * texESM(shadowTex, texSize, center + offsets.xw, shadowPos.z, esmFactor);
        sum += uw1 * vw1 * texESM(shadowTex, texSize, center + offsets.yw, shadowPos.z, esmFactor);
    }

    float fade;

    fade  = saturate(shadowPos.x / 0.01);
    fade *= saturate(shadowPos.y / 0.01);
    fade *= saturate(shadowPos.z / 0.01);
    fade *= saturate((1 - shadowPos.x) / 0.01);
    fade *= saturate((1 - shadowPos.y) / 0.01);
    fade *= saturate((1 - shadowPos.z) / 0.01);

    return lerp(1, sum / 16, fade);
}

// Welcome to Sonic Forces PC....
// ...where none of the shaders are compiled with optimizations.
// NONE.
// The function you see below evaluates to a constant value.
// And yet... Forces STILL computes it real-time.

float ComputeSggiDiffuseFactor()
{
    // Supposedly a SG integral computation...
    float4 r4, r6, r7, r9, r10;

    r4.w = 4.02;                            // mov r4.w, l(4.020000)
    r6.w = 1;                               // itof r6.w, l(1)
                                            // mov r4.w, r4.w
    r7.w = (2 * PI) / r4.w;                 // div r7.w, l(6.283185), r4.w
    r9.w = 1;                               // itof r9.w, l(1)
    r10.w = -2;                             // itof r10.w, l(-2)
    r10.w = r4.w * r10.w;                   // mul r10.w, r4.w, r10.w
    r10.w = r10.w * LOG2E;                  // mul r10.w, r10.w, l(1.442695)
    r10.w = exp2(r10.w);                    // exp r10.w, r10.w
    r10.w = -r10.w;                         // mov r10.w, -r10.w
    r9.w = r9.w + r10.w;                    // add r9.w, r9.w, r10.w
    r7.w = r7.w * r9.w;                     // mul r7.w, r7.w, r9.w
    r6.w = r6.w / r7.w;                     // div r6.w, r6.w, r7.w

    return r6.w;
}

float3 ComputeSggiDiffuse(Material material, float3 amplitude, float3 axis)
{
    float4 r9 = material.Normal.yxzy;
    float4 r12, r7, r13, r14, r15, r10;

    r12.xyz = amplitude.xyz;                // mov r12.xyz, x1[0].xyzx
    r7.w = 4;                               // mov r7.w, l(4.000000)
    r13.xyz = axis;                         // mov r13.xyz, x2[0].xyzx
    r13.w = r9.x * r13.y;                   // mul r13.w, r9.x, r13.y

    /*
    r9.w = dot(r13.xzw, r13.xzw);           // dp3 r9.w, r13.xzwx, r13.xzwx
    r9.w = rsqrt(r9.w);                     // rsq r9.w, r9.w
    r13.xyz = r9.w * r13.xwz;               // mul r13.xyz, r9.wwww, r13.xwzx,
    */
    r13.xyz = normalize(r13.xwz);

    r14.z = -r9.z;                          // mov r14.z, -r9.z
    r14.xy = r9.xy;                         // mov r14.xy, r9.xyxx
    r13.xyz = r7.w * r13.xyz;               // mul r13.xyz, r7.wwww, r13.xyzx
    r15.xyz = 4.02 * r14.yxz;               // mul r15.xyz, r4.wwww, r14.yxzy
    r13.xyz = r13.xyz + r15.xyz;            // add r13.xyz, r13.xyzx, r15.xyzx
    r7.w = dot(r13.xyz, r13.xyz);           // dp3 r7.w, r13.xyzx, r13.xyzx
    r7.w = sqrt(r7.w);                      // sqrt r7.w, r7.w
    r9.w = r7.w * LOG2E;                    // mul r9.w, r7.w, l(1.442695)
    r10.w = exp2(r9.w);                     // exp r10.w, r9.w
    r9.w = -r9.w;                           // mov r9.w, -r9.w
    r9.w = exp2(r9.w);                      // exp r9.w, r9.w
    r9.w = -r9.w;                           // mov r9.w, -r9.w
    r9.w = r9.w + r10.w;                    // add r9.w, r9.w, r10.w
    r9.w = r9.w * 0.5f;                     // mul r9.w, r9.w, l(0.500000)
    r7.w = r9.w / r7.w;                     // div r7.w, r9.w, r7.w
    r7.w = r7.w * 0.00421554781;            // mul r7.w, r7.w, l(0.004216)
    r12.xyz = r7.w * r12.xyz;               // mul r12.xyz, r7.wwww, r12.xyzx
    return r12.xyz;                         // add r10.xyz, r10.xyzx, r12.xyzx
}

float ComputeSggiSpecularFactor(Material material, out float4 r6)
{
    float4 r9 = material.Normal.yxzy;
    float4 r4, r7, r10, r11;

    r6 = 0;

    r4.w = dot(r9.yxz, material.ViewDirection); // dp3 r4.w, r9.yxzy, r5.xyzx (r5 = material.Lo)

    /*
    r6.w = 2;                                   // itof r6.w, l(2)
    r7.w = max(r4.w, 0);                        // max r7.w, r4.w, l(0.000000)
    r7.w = min(r7.w, 1);                        // min r7.w, r7.w, l(1.000000)
    r6.w = r6.w * r7.w;                         // mul r6.w, r6.w, r7.w
    r12.xyz = r9.yxz * r6.w;                    // mul r12.xyz, r9.yxzy, r6.wwww
    r13.xyz = -material.Lo;                     // mov r13.xyz, -r5.xyzx
    r12.xyz = r12.xyz + r13.xyz;                // add r12.xyz, r12.xyzx, r13.xyzx
    */

    r6.w = material.Roughness * 2.5f;           // mul r6.w, r8.x, l(2.500000) (r8.x = material.roughness)
    r7.w = -1.5f;                               // mov r7.w, l(-1.500000)
    r6.w = r6.w + r7.w;                         // add r6.w, r6.w, r7.w
    r6.w = max(r6.w, 0);                        // max r6.w, r6.w, l(0.000000)
    r6.w = min(r6.w, 1);                        // min r6.w, r6.w, l(1.000000)
    r6.w = r6.w * r6.w;                         // mul r6.w, r6.w, r6.w
    r6.w = max(r6.w, 0.005);                    // max r6.w, r6.w, l(0.005000)
    r6.w = r6.w * r6.w;                         // mul r6.w, r6.w, r6.w
    r6.w = 2.0f / r6.w;                         // div r6.w, l(2.000000), r6.w
    r7.w = 4;                                   // itof r7.w, l(4)
    r9.w = -r4.w;                               // mov r9.w, -r4.w
    r9.w = max(r4.w, r9.w);                     // max r9.w, r4.w, r9.w
    r9.w = max(r9.w, 0.05);                     // max r9.w, r9.w, l(0.050000)
    r7.w = r7.w * r9.w;                         // mul r7.w, r7.w, r9.w
    r6.w = r6.w / r7.w;                         // div r6.w, r6.w, r7.w
    r6.w = r6.w * 2;                            // mul r6.w, r6.w, l(2.000000)
    r7.w = 70;                                  // itof r7.w, l(70)
    r6.w = min(r6.w, r7.w);                     // min r6.w, r6.w, r7.w
    r7.w = 1;                                   // itof r7.w, l(1)
    r9.w = (2 * PI) / r6.w;                     // div r9.w, l(6.283185), r6.w
    r10.w = 1;                                  // itof r10.w, l(1)
    r11.w = -2;                                 // itof r11.w, l(-2)
    r11.w = r6.w * r11.w;                       // mul r11.w, r6.w, r11.w
    r11.w = r11.w * LOG2E;                      // mul r11.w, r11.w, l(1.442695)
    r11.w = exp2(r11.w);                        // exp r11.w, r11.w
    r11.w = -r11.w;                             // mov r11.w, -r11.w
    r10.w = r10.w + r11.w;                      // add r10.w, r10.w, r11.w
    r9.w = r9.w * r10.w;                        // mul r9.w, r9.w, r10.w
    return r7.w / r9.w;                         // div r7.w, r7.w, r9.w
}

float3 ComputeSggiSpecular(Material material, float3 amplitude, float3 axis, float4 r6)
{
    float4 r9 = material.Normal.yxzy;
    float4 r12 = material.SmoothReflectionDirection.xyzx;
    float4 r15, r10, r16, r11, r14;

    r14.yzw = amplitude;                    // mov r14.yzw, x1[0].xxyz
    r9.w = 4;                               // mul r9.w, r4.w, l(4.000000) (r4.w = 1.0)

    r15.xyz = axis;                         // mov r15.xyz, x6[0].xyzx

    r15.w = r9.x * r15.y;                   // mul r15.w, r14.x, r15.y (r14.x = r9.x)

    /*
    r10.w = dot(r15.xzw, r15.xzw);          // dp3 r10.w, r15.xzwx, r15.xzwx
    r10.w = rsqrt(r10.w);                   // rsq r10.w, r10.w
    r15.xyz = r10.w * r15.xwz;              // mul r15.xyz, r10.wwww, r15.xwzx
    */
    r15.xyz = normalize(r15.xwz);

    r16.z = -r12.z;                         // mov r16.z, -r12.z
    r16.xy = r12.xy;                        // mov r16.xy, r12.xyxx
    r12.xyw = r9.w * r15.xyz;               // mul r12.xyw, r9.wwww, r15.xyxz
    r15.xyz = r6.w * r16.xyz;               // mul r15.xyz, r6.wwww, r16.xyzx
    r12.xyw = r12.xyw + r15.xyz;            // add r12.xyw, r12.xyxw, r15.xyxz
    r10.w = dot(r12.xyw, r12.xyw);          // dp3 r10.w, r12.xywx, r12.xywx
    r10.w = sqrt(r10.w);                    // sqrt r10.w, r10.w
    r9.w = r6.w + r9.w;                     // add r9.w, r6.w, r9.w
    r11.w = -r10.w;                         // mov r11.w, -r10.w
    r9.w = r9.w + r11.w;                    // add r9.w, r9.w, r11.w
    r9.w = r9.w * LOG2E;                    // mul r9.w, r9.w, l(1.442695)
    r9.w = exp2(r9.w);                      // exp r9.w, r9.w
    r9.w = (4 * PI) / r9.w;                 // div r9.w, l(12.566371), r9.w
    r11.w = 1;                              // itof r11.w, l(1)
    r12.x = -2;                             // itof r12.x, l(-2)
    r12.x = r10.w * r12.x;                  // mul r12.x, r10.w, r12.x
    r12.x = r12.x * LOG2E;                  // mul r12.x, r12.x, l(1.442695)
    r12.x = exp2(r12.x);                    // exp r12.x, r12.x
    r12.x = -r12.x;                         // mov r12.x, -r12.x
    r11.w = r11.w + r12.x;                  // add r11.w, r11.w, r12.x
    r11.w = r11.w * 0.5;                    // mul r11.w, r11.w, l(0.500000)
    r10.w = r11.w / r10.w;                  // div r10.w, r11.w, r10.w
    r9.w = r9.w * r10.w;                    // mul r9.w, r9.w, r10.w
    r12.xyw = r9.w * r14.yzw;               // mul r12.xyw, r9.wwww, r14.yzyw
    return r12.xyw;                         // add r11.xyz, r11.xyzx, r12.xywx
}

bool IsInSHCoordRange(float3 shCoords)
{
    return 0.5 >= abs(shCoords.x) && 0.5 >= abs(shCoords.y) && 0.5 >= abs(shCoords.z);
}

float3 ComputeSHTexCoords(float3 shCoords, float4 shParam)
{
    float4 r19 = shParam;
    float4 r20 = shCoords.xyzx;
    float4 r15, r10, r3;

    r15.xyz = r20.xyz;                      // div r15.xyz, r20.xyzx, l(1.000000, 1.000000, 1.000000, 0.000000)
    r15.xyz = r15.xyz + 0.5;                // add r15.xyz, r15.xyzx, l(0.500000, 0.500000, 0.500000, 0.000000)
    r15.xyz = max(r15.xyz, 0);              // max r15.xyz, r15.xyzx, l(0.000000, 0.000000, 0.000000, 0.000000)
    r15.xyz = min(r15.xyz, 1);              // min r15.xyz, r15.xyzx, l(1.000000, 1.000000, 1.000000, 0.000000)

    // We will be passing it like this
    // in the constant buffer.
    // r19.xyz = 1.0 / r19.xyz;             // div r19.xyz, l(1.000000, 1.000000, 1.000000, 0.000000), r19.xyzx
    // r19.xyz = r19.xyz * 0.5;             // mul r19.xyz, r19.xyzx, l(0.500000, 0.500000, 0.500000, 0.000000)

    r20.xyz = -r19.xyz;                     // mov r20.xyz, -r19.xyzx
    r20.xyz = r20.xyz + 1;                  // add r20.xyz, r20.xyzx, l(1.000000, 1.000000, 1.000000, 0.000000)
    r15.xyz = max(r15.xyz, r19.xyz);        // max r15.xyz, r15.xyzx, r19.xyzx
    r15.xyz = min(r20.xyz, r15.xyz);        // min r15.xyz, r20.xyzx, r15.xyzx
    r3.x = 9;                               // itof r3.x, l(9)
    r15.x = r15.x / r3.x;                   // div r15.x, r15.x, r3.x

    return r15.xyz;
}

float3 ComputeSHBasis(Material material, float3 amplitude, float3 axis)
{
    float4 r18 = material.Normal.xyzx;
    float4 r3 = -material.Normal.z;

    float4 r21, r12, r13, r22, r23, r14, r20, r15, r16;

                                            // ilt r12.w, r10.w, l(6)
                                            // breakc_z r12.w
    r20.xyz = amplitude;                    // mov r20.xyz, x1[r10.w + 0].xyzx
    r21.xyz = 1.17;                         // mov r21.xyz, l(1.170000,1.170000,1.170000,0)
    r12.w = 3;                              // mov r12.w, l(3.000000)
    r13.w = 2.133;                          // mov r13.w, l(2.133000)
    r22.xyz = axis;                         // mov r22.xyz, x2[r10.w + 0].xyzx
    r23.xy = r18.xy;                        // mov r23.xy, r18.xyxx
    r23.z = r3.x;                           // mov r23.z, r3.x
    r22.xyz = r12.w * r22.xyz;              // mul r22.xyz, r12.wwww, r22.xyzx
    r23.xyz = r13.w * r23.xyz;              // mul r23.xyz, r13.wwww, r23.xyzx
    r22.xyz = r22.xyz + r23.xyz;            // add r22.xyz, r22.xyzx, r23.xyzx
    r14.w = dot(r22.xyz, r22.xyz);          // dp3 r14.w, r22.xyzx, r22.xyzx
    r14.w = sqrt(r14.w);                    // sqrt r14.w, r14.w
    r20.xyz = r20.xyz * (4 * PI);           // mul r20.xyz, r20.xyzx, l(12.566371, 12.566371, 12.566371, 0.000000)
    r20.xyz = r21.xyz * r20.xyz;            // mul r20.xyz, r21.xyzx, r20.xyzx
    r15.w = r14.w * LOG2E;                  // mul r15.w, r14.w, l(1.442695)
    r16.x = exp2(r15.w);                    // exp r16.x, r15.w
    r15.w = -r15.w;                         // mov r15.w, -r15.w
    r15.w = exp2(r15.w);                    // exp r15.w, r15.w
    r15.w = -r15.w;                         // mov r15.w, -r15.w
    r15.w = r15.w + r16.x;                  // add r15.w, r15.w, r16.x
    r15.w = r15.w * 0.5;                    // mul r15.w, r15.w, l(0.500000)
    r20.xyz = r15.w * r20.xyz;              // mul r20.xyz, r15.wwww, r20.xyzx
    r12.w = r12.w + r13.w;                  // add r12.w, r12.w, r13.w
    r12.w = r12.w * LOG2E;                  // mul r12.w, r12.w, l(1.442695)
    r12.w = exp2(r12.w);                    // exp r12.w, r12.w
    r12.w = r14.w * r12.w;                  // mul r12.w, r14.w, r12.w
    r20.xyz = r20.xyz / r12.w;              // div r20.xyz, r20.xyzx, r12.wwww
    return r20.xyz;                         // add r19.yzw, r19.yyzw, r20.xxyz
                                            // iadd r10.w, r10.w, l(1)
}

float3 ComputeSHFinal(float3 sum)
{
    return max(0, sum / PI);
}

float3 ComputeCharaHighlight(Material material, float3 color, float threshold, float ambientScale, float albedoHeighten, float falloffScale)
{
    float4 r12 = material.Albedo.xyzx;
    float4 r15 = color.xyzx;

    float4 r0, r5, r6, r16, r9;

    r0.z = max(r15.x, r15.y);
    r0.z = max(r0.z, r15.z);

    if (r0.z < threshold)
    {
        r0.z = r0.z / threshold;
        r0.z = max(0, r0.z);
        r0.z = min(1, r0.z);
        r5.w = 1;
        r6.w = -ambientScale;
        r5.w = r6.w + r5.w;
        r5.w = r5.w * r0.z;
        r5.w = ambientScale + r5.w;
        r15.xyz = r15.xyz * r5.www;
        r5.w = 0;
        r6.w = -albedoHeighten;
        r5.w = r6.w + r5.w;
        r5.w = r5.w * r0.z;
        r5.w = albedoHeighten + r5.w;
        r16.xyz = r12.xyz + r5.www;
        r5.w = 0.5;
        r6.w = falloffScale;
        r9.x = material.CosViewDirection;
        r9.x = -r9.x;
        r9.x = 1 + r9.x;
        r9.x = max(0, r9.x);
        r9.x = min(1, r9.x);
        r9.y = -r5.w;
        r9.x = r9.x + r9.y;
        r5.w = -r5.w;
        r5.w = 1 + r5.w;
        r5.w = r9.x / r5.w;
        r5.w = max(0, r5.w);
        r5.w = min(1, r5.w);
        r9.x = 1;
        r5.w = r5.w * r5.w;
        r5.w = r9.x * r5.w;
        r5.w = r6.w * r5.w;
        r5.w = r5.w;
        r6.w = 0;
        r9.x = -r5.w;
        r6.w = r9.x + r6.w;
        r0.z = r6.w * r0.z;
        r0.z = r5.w + r0.z;
        r12.xyz = r16.xyz + r0.zzz;
    }

    return r12.xyz;
}

float3 ComputeRoughReflectionDirection(float roughness, float3 normal, float3 viewDirection)
{
    float3 r11 = roughness;
    float3 r12 = normal.yxz;
    float3 r13 = viewDirection;

    float3 r3, r19, r5;

    r3.x = dot(r12.yxz, r13.xyz);
    r3.x = max(0, r3.x);
    r3.x = min(1, r3.x);
    r3.y = 2;
    r3.y = r3.y * r3.x;
    r19.xyz = r3.yyy * r12.yxz;
    r13.xyz = -r13.xyz;
    r13.xyz = r19.xyz + r13.xyz;
    r3.y = dot(r13.xyz, r13.xyz);
    r3.y = rsqrt(r3.y);
    r13.xyz = r13.xyz * r3.yyy;
    r3.y = 1;
    r5.x = -r11.x;
    r3.y = r5.x + r3.y;
    r3.y = max(0, r3.y);
    r3.y = min(1, r3.y);
    r5.x = sqrt(r3.y);
    r5.x = r5.x + r11.x;
    r3.y = r5.x * r3.y;
    r19.xyz = -r12.yxz;
    r13.xyz = r19.xyz + r13.xyz;
    r13.xyz = r13.xyz * r3.yyy;
    r12.xyz = r13.xyz + r12.yxz;

    return r12;
}

float ComputeIndirectIBLFade(float roughness)
{
    return 1.0 / min(0.5, roughness);
}

float3 GetPositionFromDepth(float2 vPos, float depth, in float4x4 invProjection)
{
    float4 position = mul(float4(vPos, depth, 1), invProjection);
    return position.xyz / position.w;
}

float3 UnpackHDR(float4 color)
{
    // rgb * exp(a * 16 - 4)
    return color.rgb * exp2(color.a * (16 * LOG2E) - (4 * LOG2E));
}

float LinearizeDepth(float depth, in float4x4 invProjection)
{
    float4 position = mul(float4(0, 0, depth, 1), invProjection);
    return position.z / position.w;
}

float3 UnpackHDRCustom(float3 color, float factor)
{
    return max(0, (factor * color) / (1 - color));
}

float3 Hash(float3 a)
{
    a = frac(a * 0.8);
    a += dot(a, a.yxz + 19.19);
    return frac((a.xxy + a.yxx) * a.zyx);
}

float PerspectiveCorrectLerp(float a, float b, float t)
{
    return (a * b) / lerp(b, a, t);
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

float ComputeFalloff(float cosViewDirection, float3 falloffFactor)
{
    return saturate(exp2(log2(saturate(1 - cosViewDirection + falloffFactor.z)) * falloffFactor.y) * falloffFactor.x);
}

float InterleavedGradientNoise(float2 p)
{
    float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(magic.z * frac(dot(p, magic.xy)));
}

float2 CalculateVogelDiskSample(int sampleIndex, int sampleCount, float phi)
{
    const float goldenAngle = 2.4;

    float r = sqrt(sampleIndex + 0.5) / sqrt(sampleCount);
    float theta = sampleIndex * goldenAngle + phi;

    float sine, cosine;
    sincos(theta, sine, cosine);

    return float2(r * cosine, r * sine);
}

#endif