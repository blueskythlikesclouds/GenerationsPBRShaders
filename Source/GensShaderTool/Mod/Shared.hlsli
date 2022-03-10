#ifndef SHARED_HLSLI_INCLUDED
#define SHARED_HLSLI_INCLUDED

#define PI       3.14159265358979323846f
#define LOG2E    1.44269504088896340736f

#ifdef ConstTexCoord

#define UV(x) \
    (input.TexCoord0.xy)

#else

#define UV(x) \
    GetTexCoord(input.TexCoord0, input.TexCoord1, x, mrgTexcoordIndex)

#endif

#ifndef GLOBALS_ONLY_PBR_CONSTANTS

cbuffer cbGlobalsShared : register(b1)
{
    uint g_Booleans;
    bool g_EnableAlphaTest;
    float g_AlphaThreshold;
}

#endif

struct VertexDeclaration
{
    float4 Position : POSITION;

#ifndef HasFeatureNoBone
    float4 BlendWeight : BLENDWEIGHT;
    uint4 BlendIndices : BLENDINDICES;
#endif

    float4 Normal : NORMAL;
    float4 TexCoord0 : TEXCOORD0;
    float4 TexCoord1 : TEXCOORD1;

#ifndef ConstTexCoord
    float4 TexCoord2 : TEXCOORD2;
    float4 TexCoord3 : TEXCOORD3;
#endif

#ifdef HasFeatureNormalMapping
    float4 Tangent : TANGENT;
    float4 Binormal : BINORMAL;
#endif

    float4 Color : COLOR;
};

struct PixelDeclaration
{
    float4 SVPosition : SV_POSITION;
    float3 Position : POSITION;
    float4 TexCoord0 : TEXCOORD0;

#ifndef ConstTexCoord
    float4 TexCoord1 : TEXCOORD1;
#endif

    float3 Normal : NORMAL;

#ifdef HasFeatureNormalMapping
    float3 Tangent : TANGENT;
    float3 Binormal : BINORMAL;
#endif

#ifdef HasFeatureEyeNormal
    float3 EyeNormal : EYENORMAL;
#endif

#ifndef HasFeatureDeferred
    float4 ShadowMapCoord : SHADOWMAPCOORD;
    float2 LightScattering : LIGHTSCATTERING;
#endif

    float4 Color : COLOR;
};

#define DEFERRED_FLAGS_GI                    (1 << 0)
#define DEFERRED_FLAGS_LIGHT_FIELD           (1 << 1)
#define DEFERRED_FLAGS_CDR                   (1 << 2)
#define DEFERRED_FLAGS_LIGHT                 (1 << 3)
#define DEFERRED_FLAGS_IBL                   (1 << 4)
#define DEFERRED_FLAGS_LIGHT_SCATTERING      (1 << 5)
#define DEFERRED_FLAGS_MAX                  ((1 << 6) - 1)

struct ShaderParams
{
    float3 Albedo;
    float Alpha;
    float Reflectance;
    float Roughness;
    float AmbientOcclusion;
    float Metalness;

    float2 NormalMap;
    float3 Cdr;
    float3 Emission;

    float4 Refraction;

    uint DeferredFlags;

    float3 IndirectDiffuse;
    float3 IndirectSpecular;
    float Shadow;

    float3 FresnelReflectance;
    float3 Normal;

    float3 ViewDirection;
    float CosViewDirection;
    float3 RoughReflectionDirection;
    float3 SmoothReflectionDirection;
};

ShaderParams CreateShaderParams()
{
    ShaderParams params;

    params.Albedo = 1.0;
    params.Alpha = 1.0;
    params.Reflectance = 0.04;
    params.Roughness = 0.5;
    params.AmbientOcclusion = 1.0;
    params.Metalness = 0.0;
    params.NormalMap = 0.0;
    params.Cdr = 1.0;
    params.Emission = 0.0;
    params.Refraction = 0.0;
    params.DeferredFlags = 0;
    params.FresnelReflectance = 0.0;
    params.IndirectDiffuse = 0.0;
    params.IndirectSpecular = 0.0;
    params.Shadow = 1.0;
    params.Normal = 0.0;
    params.ViewDirection = 0.0;
    params.CosViewDirection = 0.0;
    params.RoughReflectionDirection = 0.0;
    params.SmoothReflectionDirection = 0.0;

    return params;
}

float2 GetTexCoord(float4 texCoord0, float4 texCoord1, uint index, float4 mrgTexcoordIndex[4])
{
#ifdef ConstTexCoord
    return texCoord0.xy;
#else

    float value = trunc(mrgTexcoordIndex[index / 4][index % 4]);

    if (value == 0.0f) return texCoord0.xy;
    if (value == 1.0f) return texCoord0.zw;
    if (value == 2.0f) return texCoord1.xy;
    if (value == 3.0f) return texCoord1.zw;

    return 0.0f;
#endif
}

float3 SrgbToLinear(float3 value)
{
    return pow(abs(value), 2.2);
}

float4 SrgbToLinear(float4 value)
{
    return float4(SrgbToLinear(value.rgb), value.a);
}

float3 LinearToSrgb(float3 value)
{
    return pow(abs(value), 1.0 / 2.2);
}

float4 LinearToSrgb(float4 value)
{
    return float4(LinearToSrgb(value.rgb), value.a);
}

void ConvertSpecularToParams(float4 specular, bool explicitMetalness, inout ShaderParams params)
{
    params.Reflectance = specular.r / 4.0;
    params.Roughness = max(0.01, 1.0 - specular.g);
    params.AmbientOcclusion = specular.b;
    params.Metalness = !explicitMetalness ? specular.r > 0.9 : specular.a;
}

void ConvertPBRFactorToParams(float4 pbrFactor, inout ShaderParams params)
{
    params.Reflectance = pbrFactor.r;
    params.Roughness = max(0.01, 1.0 - pbrFactor.g);
    params.AmbientOcclusion = 1.0;
    params.Metalness = pbrFactor.r > 0.9;
}

float ComputeFalloff(float cosViewDirection, float3 falloffFactor)
{
    return saturate(exp2(log2(saturate(1 - cosViewDirection + falloffFactor.z)) * falloffFactor.y) * falloffFactor.x);
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

float3 ComputeSggiDiffuse(in ShaderParams params, float3 amplitude, float3 axis)
{
    float4 r9 = params.Normal.yxzy;
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

float ComputeSggiSpecularFactor(in ShaderParams params, out float4 r6)
{
    float4 r9 = params.Normal.yxzy;
    float4 r4, r7, r10, r11;

    r6 = 0;

    r4.w = dot(r9.yxz, params.ViewDirection);   // dp3 r4.w, r9.yxzy, r5.xyzx (r5 = material.Lo)

    /*
    r6.w = 2;                                   // itof r6.w, l(2)
    r7.w = max(r4.w, 0);                        // max r7.w, r4.w, l(0.000000)
    r7.w = min(r7.w, 1);                        // min r7.w, r7.w, l(1.000000)
    r6.w = r6.w * r7.w;                         // mul r6.w, r6.w, r7.w
    r12.xyz = r9.yxz * r6.w;                    // mul r12.xyz, r9.yxzy, r6.wwww
    r13.xyz = -material.Lo;                     // mov r13.xyz, -r5.xyzx
    r12.xyz = r12.xyz + r13.xyz;                // add r12.xyz, r12.xyzx, r13.xyzx
    */

    r6.w = params.Roughness * 2.5f;             // mul r6.w, r8.x, l(2.500000) (r8.x = material.roughness)
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

float3 ComputeSggiSpecular(in ShaderParams params, float3 amplitude, float3 axis, float4 r6)
{
    float4 r9 = params.Normal.yxzy;
    float4 r12 = params.SmoothReflectionDirection.xyzx;
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

float3 ComputeSHTexCoords(float3 shCoords, float3 shParam)
{
    float4 r19 = shParam.xyzx;
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

float3 ComputeSHBasis(in ShaderParams params, float3 amplitude, float3 axis)
{
    float4 r18 = params.Normal.xyzx;
    float4 r3 = -params.Normal.z;

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

float3 UnpackHDR(float4 color)
{
    // rgb * exp(a * 16 - 4)
    return color.rgb * exp2(color.a * (16 * LOG2E) - (4 * LOG2E));
}

float2 ApproxEnvBRDF(float cosLo, float roughness)
{
    float4 c0 = float4(-1, -0.0275, -0.572, 0.022);
    float4 c1 = float4(1, 0.0425, 1.04, -0.04);
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * cosLo)) * r.x + r.y;
    return float2(-1.04, 1.04) * a004 + r.zw;
}

float3 ComputeIndirectLighting(in ShaderParams params, float2 specularBRDF)
{
    float3 kd = lerp(1 - params.FresnelReflectance, 0, params.Metalness);

    float3 diffuseIBL = (kd * params.Albedo) * params.IndirectDiffuse;

    float3 specularIBL = (params.FresnelReflectance * specularBRDF.x + specularBRDF.y) * params.IndirectSpecular;

    return (diffuseIBL + specularIBL) * params.AmbientOcclusion;
}

float3 ComputeIndirectLighting(in ShaderParams material)
{
    return ComputeIndirectLighting(material, ApproxEnvBRDF(material.CosViewDirection, material.Roughness));
}

float3 ComputeIndirectLighting(in ShaderParams material, Texture2D<float2> texEnvBRDF, SamplerState sampEnvBRDF)
{
    return ComputeIndirectLighting(material, texEnvBRDF.Sample(sampEnvBRDF, float2(material.CosViewDirection, material.Roughness)));
}

float4 ComputeOcclusion(float4 values, float depth, float esmFactor)
{
    return saturate(exp2((values - depth) * esmFactor));
}

float SampleShadow(Texture2D<float> texShadow, SamplerState sampShadow, float2 texSize, float esmFactor, float2 pixelPos, float depth)
{
    float2 texCoord = (pixelPos - 0.5) * texSize.y;
    float2 fraction = frac(pixelPos);

    float4 values = ComputeOcclusion(texShadow.GatherRed(sampShadow, texCoord), depth, esmFactor);
    return lerp(lerp(values.w, values.z, fraction.x), lerp(values.x, values.y, fraction.x), fraction.y);
}

float ComputeShadow(Texture2D<float> texShadow, SamplerState sampShadow, float2 texSize, float esmFactor, float4 shadowPos)
{
    if (any(abs(shadowPos.xyz) >= shadowPos.w))
        return 1.0;

    shadowPos /= shadowPos.w;
    shadowPos.xy = (shadowPos.xy * float2(0.5, -0.5) + 0.5) * texSize.x;

    // From: https://github.com/TheRealMJP/Shadows

    float2 basePos;
    basePos.x = floor(shadowPos.x + 0.5);
    basePos.y = floor(shadowPos.y + 0.5);

    float s = (shadowPos.x + 0.5 - basePos.x);
    float t = (shadowPos.y + 0.5 - basePos.y);

    float uw0 = (4 - 3 * s);
    float uw1 = 7;
    float uw2 = (1 + 3 * s);

    float u0 = (3 - 2 * s) / uw0 - 2;
    float u1 = (3 + s) / uw1;
    float u2 = s / uw2 + 2;

    float vw0 = (4 - 3 * t);
    float vw1 = 7;
    float vw2 = (1 + 3 * t);

    float v0 = (3 - 2 * t) / vw0 - 2;
    float v1 = (3 + t) / vw1;
    float v2 = t / vw2 + 2;

    float sum = 0.0;

    sum += uw0 * vw0 * SampleShadow(texShadow, sampShadow, texSize, esmFactor, basePos + float2(u0, v0), shadowPos.z);
    sum += uw1 * vw0 * SampleShadow(texShadow, sampShadow, texSize, esmFactor, basePos + float2(u1, v0), shadowPos.z);
    sum += uw2 * vw0 * SampleShadow(texShadow, sampShadow, texSize, esmFactor, basePos + float2(u2, v0), shadowPos.z);
    sum += uw0 * vw1 * SampleShadow(texShadow, sampShadow, texSize, esmFactor, basePos + float2(u0, v1), shadowPos.z);
    sum += uw1 * vw1 * SampleShadow(texShadow, sampShadow, texSize, esmFactor, basePos + float2(u1, v1), shadowPos.z);
    sum += uw2 * vw1 * SampleShadow(texShadow, sampShadow, texSize, esmFactor, basePos + float2(u2, v1), shadowPos.z);
    sum += uw0 * vw2 * SampleShadow(texShadow, sampShadow, texSize, esmFactor, basePos + float2(u0, v2), shadowPos.z);
    sum += uw1 * vw2 * SampleShadow(texShadow, sampShadow, texSize, esmFactor, basePos + float2(u1, v2), shadowPos.z);
    sum += uw2 * vw2 * SampleShadow(texShadow, sampShadow, texSize, esmFactor, basePos + float2(u2, v2), shadowPos.z);

    return sum / 144.0;
}

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

float3 ComputeDirectLighting(in ShaderParams params, float3 lightDirection, float3 lightColor, bool cdr = false)
{
    float3 halfwayDirection = normalize(params.ViewDirection + lightDirection);

    float cosLightDirection = saturate(dot(lightDirection, params.Normal));
    float cosHalfwayDirection = saturate(dot(halfwayDirection, params.Normal));

    float3 F = FresnelSchlick(params.FresnelReflectance, saturate(dot(halfwayDirection, params.ViewDirection)));
    float D = NdfGGX(cosHalfwayDirection, params.Roughness);
    float Vis = VisSchlick(params.Roughness, params.CosViewDirection, cosLightDirection);

    float3 kd = lerp(1 - F, 0, params.Metalness);

    float3 diffuseBRDF = kd * (params.Albedo / PI);
    float3 specularBRDF = (D * Vis) * F;

    float3 directLighting = (diffuseBRDF + specularBRDF) * lightColor;
    if (cdr)
        directLighting *= params.Cdr;
    else
        directLighting *= saturate(dot(lightDirection, params.Normal));

    return directLighting;
}

float3 ComputeLocalLight(float3 position, in ShaderParams params, float3 lightPosition, float3 lightColor, float2 lightRange)
{
    float3 delta = lightPosition - position;
    float distanceSqr = dot(delta, delta);

    if (distanceSqr > lightRange.x)
        return 0;

    float lightMask = distanceSqr * lightRange.y;
    lightMask = saturate(1 - lightMask * lightMask);
    lightMask *= lightMask;

    float lightAttenuation = 1.0f / max(1, distanceSqr);

    return ComputeDirectLighting(params, delta * rsqrt(distanceSqr), lightColor) * lightMask * lightAttenuation;
}

float3 GetPositionFromDepth(float2 vPos, float depth, in float4x4 invProjection)
{
    float4 position = mul(float4(vPos, depth, 1), invProjection);
    return position.xyz / position.w;
}

float LinearizeDepth(float depth, in float4x4 invProjection)
{
    float4 position = mul(float4(0, 0, depth, 1), invProjection);
    return position.z / position.w;
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

float PerspectiveCorrectLerp(float a, float b, float t)
{
    return (a * b) / lerp(b, a, t);
}

#endif