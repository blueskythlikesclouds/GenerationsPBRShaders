float4 mrgGlobalLight_Direction : register(c10);
float4 g_Diffuse : register(c16);
float4 g_Ambient : register(c17);
float4 g_Specular : register(c18);
float4 g_Emissive : register(c19);
float4 g_PowerGlossLevel : register(c20);
float4 g_OpacityReflectionRefractionSpectype : register(c21);
float4 g_EyePosition : register(c22);
float4 g_ViewportSize : register(c24);
float4 mrgGlobalLight_Diffuse : register(c36);
float4 mrgGlobalLight_Specular : register(c37);
float4 mrgLocalLight1_Position : register(c42);
float4 mrgLocalLight1_Color : register(c43);
float4 mrgLocalLight1_Range : register(c44);
float4 mrgLocalLight2_Position : register(c46);
float4 mrgLocalLight2_Color : register(c47);
float4 mrgLocalLight2_Range : register(c48);
float4 mrgLocalLight3_Position : register(c50);
float4 mrgLocalLight3_Color : register(c51);
float4 mrgLocalLight3_Range : register(c52);
float4 mrgLocalLight4_Position : register(c54);
float4 mrgLocalLight4_Color : register(c55);
float4 mrgLocalLight4_Range : register(c56);
float4 mrgEyeLight_Diffuse : register(c58);
float4 mrgEyeLight_Specular : register(c59);
float4 mrgEyeLight_Range : register(c60);
float4 mrgLuminanceRange : register(c63);
float4 mrgInShadowScale : register(c64);
float4 g_GIModeParam : register(c69);
float4 g_aLightField[6] : register(c77);
row_major float4x4 g_MtxView : register(c90);
float4 mrgInShadowScaleLF : register(c146);

sampler2D sampDiffuse : register(s0);
sampler2D sampSpecular : register(s1);
sampler2D sampNormal : register(s2);

#ifdef HasSamplerReflection2
samplerCUBE sampReflection : register(s3);
#else
sampler2D sampReflection : register(s3);
#endif

sampler2D sampDLScatter : register(s9);
sampler2D g_FramebufferSampler : register(s11);

struct PSInput
{
    float2 VPos : VPOS;
    float4 TexCoord : TEXCOORD0;
    float4 Position : TEXCOORD2;
    float4 Normal : TEXCOORD3; // fresnel in W
    float4 LightScattering : TEXCOORD5;
    float4 Tangent : TEXCOORD7;
    float4 Binormal : TEXCOORD8;
    float4 Color : COLOR;
};

float3 ComputeDirectLighting(
    float3 position,
    float3 diffuse, 
    float3 specular, 
    float specularPower,
    float3 normal,
    float3 lightDirection,
    float3 lightDiffuse,
    float3 lightSpecular)
{
    float3 eyeDirection = normalize(g_EyePosition.xyz - position.xyz);
    float3 halfwayDirection = normalize(eyeDirection + lightDirection);
    
    float nDotL = saturate(dot(normal, lightDirection));
    diffuse *= lightDiffuse * nDotL;
    specular *= lightSpecular * pow(saturate(dot(normal, halfwayDirection)), specularPower) * pow(nDotL, 8.0);
    
    return diffuse + specular;
}

float3 ComputeLocalLighting(
    float3 position,
    float3 diffuse,
    float3 specular,
    float specularPower,
    float3 normal,
    float3 lightPosition,
    float3 lightDiffuse,
    float3 lightSpecular,
    float2 lightRange)
{
    float3 lightDirection = lightPosition - position;
    float distance = length(lightDirection);
    lightDirection /= distance;
    
    float3 localLighting = ComputeDirectLighting(
        position,
        diffuse,
        specular,
        specularPower,
        normal,
        lightDirection,
        lightDiffuse,
        lightSpecular);
    
    localLighting *= saturate(lightRange.x * -distance + lightRange.y);
    return localLighting;
}

float2 ComputeEnvMapTexCoord(float3 eyeDirection, float3 normal)
{
    float4 C[4];
    C[0] = float4(0.5, 500, 5, 1);
    C[1] = float4(1024, 0, -2, 3);
    C[2] = float4(0.25, 4, 0, 0);
    C[3] = float4(-1, 1, 0, 0.5);
    float4 r3 = eyeDirection.xyzx;
    float4 r4 = normal.xyzx;
    float4 r1, r2, r6;
    r1.w = dot(r3.yzw, r4.yzw);
    r1.w = r1.w + r1.w;
    r2 = r1.wwww * r4.xyzw + -r3.xyzw;
    r3 = r2.wyzw * C[3].xxyz + C[3].zzzw;
    r6 = r2.xyzw * C[3].yxxz;
    r2 = r2.zzzz >= 0 ? r3.xyzw : r6.xyzw;
    r1.w = r2.z + C[0].w;
    r1.w = 1.0 / r1.w;
    r2.xy = r2.yx * r1.ww + C[0].ww;
    r3.x = r2.y * C[2].x + r2.w;
    r3.y = r2.x * C[0].x;
    return r3.xy;
}

float4 main(in PSInput input) : COLOR
{
    float4 diffuse = tex2D(sampDiffuse, input.TexCoord.xy);
    
#ifdef HasSamplerSpecular
    float4 specular = tex2D(sampSpecular, input.TexCoord.xy);
#else
    float4 specular = 1.0;
#endif    
    
#ifdef HasSamplerNormal
    float4 normalMap = tex2D(sampNormal, input.TexCoord.xy);

    normalMap.x *= normalMap.w;
    normalMap.xy = normalMap.xy * 2.0 - 1.0;
    normalMap.z = sqrt(1.0 - saturate(dot(normalMap.xy, normalMap.xy)));
   
    float3 normal = normalize(
        normalMap.x * normalize(input.Tangent.xyz) +
        normalMap.y * normalize(input.Binormal.xyz) +
        normalMap.z * normalize(input.Normal.xyz));
#else
    float3 normal = normalize(input.Normal.xyz);
#endif
    
    float3 eyeDirection = normalize(g_EyePosition.xyz - input.Position.xyz);
    float fresnel = (1.0 - saturate(dot(eyeDirection, normal))) * g_Emissive.y;

    diffuse.rgb *= lerp(g_Diffuse.rgb, g_Ambient.rgb, fresnel);
    specular.rgb *= g_Specular.rgb;
    
    float3 directLighting = ComputeDirectLighting(
        input.Position.xyz,
        diffuse.rgb,
        specular.rgb,
        g_PowerGlossLevel.x,
        normal,
        -mrgGlobalLight_Direction.xyz,
        mrgGlobalLight_Diffuse.rgb,
        mrgGlobalLight_Specular.rgb) * g_aLightField[0].w;
    
    float3 eyeLighting = ComputeLocalLighting(
        input.Position.xyz,
        diffuse.rgb,
        specular.rgb,
        g_PowerGlossLevel.x,
        normal,
        g_EyePosition.xyz,
        mrgEyeLight_Diffuse.rgb,
        mrgEyeLight_Specular.rgb,
        mrgEyeLight_Range.zw);
    
    float3 localLighting = 0.0;
    
#ifndef NoLight
    localLighting += ComputeLocalLighting(
        input.Position.xyz,
        diffuse.rgb,
        specular.rgb,
        g_PowerGlossLevel.x,
        normal,
        mrgLocalLight1_Position.xyz,
        mrgLocalLight1_Color.rgb,
        mrgLocalLight1_Color.rgb,
        mrgLocalLight1_Range.zw);
    
    localLighting += ComputeLocalLighting(
        input.Position.xyz,
        diffuse.rgb,
        specular.rgb,
        g_PowerGlossLevel.x,
        normal,
        mrgLocalLight2_Position.xyz,
        mrgLocalLight2_Color.rgb,
        mrgLocalLight2_Color.rgb,
        mrgLocalLight2_Range.zw);
    
    localLighting += ComputeLocalLighting(
        input.Position.xyz,
        diffuse.rgb,
        specular.rgb,
        g_PowerGlossLevel.x,
        normal,
        mrgLocalLight3_Position.xyz,
        mrgLocalLight3_Color.rgb,
        mrgLocalLight3_Color.rgb,
        mrgLocalLight3_Range.zw);
    
    localLighting += ComputeLocalLighting(
        input.Position.xyz,
        diffuse.rgb,
        specular.rgb,
        g_PowerGlossLevel.x,
        normal,
        mrgLocalLight4_Position.xyz,
        mrgLocalLight4_Color.rgb,
        mrgLocalLight4_Color.rgb,
        mrgLocalLight4_Range.zw);
#endif
    
    float3 lightField = diffuse.rgb * (
        (g_aLightField[0].rgb + g_aLightField[1].rgb * (normal.x * 0.5 + 0.5)) * normal.x * normal.x +
        (g_aLightField[2].rgb + g_aLightField[3].rgb * (normal.y * 0.5 + 0.5)) * normal.y * normal.y +
        (g_aLightField[4].rgb + g_aLightField[5].rgb * (normal.z * 0.5 + 0.5)) * normal.z * normal.z);
    
    float3 reflection = 0.0;

#if defined(HasSamplerReflection)
    reflection = tex2D(sampReflection, ComputeEnvMapTexCoord(eyeDirection, normal)).rgb;
#elif defined(HasSamplerReflection1)
    float3 viewNormal = mul(float4(normal, 0.0), g_MtxView).xyz;
    reflection = tex2D(sampReflection, viewNormal.xy * float2(0.5, -0.5) + 0.5).rgb;
#elif defined(HasSamplerReflection2)
    reflection = texCUBE(sampReflection, reflect(-eyeDirection, normal)).rgb;
#endif
    
    float3 envMapLighting = (reflection * mrgLuminanceRange.x * specular.w);
    envMapLighting *= g_aLightField[0].w * (mrgInShadowScale.x * -mrgInShadowScale.z + mrgInShadowScale.z) + mrgInShadowScale.z * mrgInShadowScale.x;

    float3 compositedLighting = (directLighting + eyeLighting + localLighting + lightField + envMapLighting) * input.Color.rgb;
    float4 directLightScatter = tex2D(sampDLScatter, input.LightScattering.yx);
    compositedLighting = directLightScatter.rgb + compositedLighting * directLightScatter.a;
    compositedLighting = lerp(compositedLighting, tex2D(g_FramebufferSampler, (input.VPos + 0.5) * g_ViewportSize.zw).rgb, input.LightScattering.x);
    
    return float4(compositedLighting, diffuse.a * input.Color.a);
}