#include "HeightMapShared.hlsli"
#include "SharedPS.hlsli"

Texture2D<float4> texSplatMap : register(t0);
Texture2D<float4> texNrm : register(t1);
Texture2DArray<float4> texDetailAbd : register(t2);
Texture2DArray<float4> texDetailNrm : register(t3);

SamplerState sampSplatMap : register(s0);
SamplerState sampNrm : register(s1);
SamplerState sampDetailAbd : register(s2);
SamplerState sampDetailNrm : register(s3);

void main(in HeightMapPSDeclaration input,
    out float4 gBuffer0 : SV_TARGET0,
    out float4 gBuffer1 : SV_TARGET1,
    out float4 gBuffer2 : SV_TARGET2,
    out float4 gBuffer3 : SV_TARGET3)
{
    float2 matUV = input.TexCoord * 1000.0;
    uint4 matIndex = uint4(texSplatMap.GatherRed(sampSplatMap, input.TexCoord) * 255.0 + 0.5);
    float3 matAlbedo[4];
    float4 matNormalMap[4];

    [unroll] for (uint i = 0; i < 4; i++)
    {
        matAlbedo[i] = pow(abs(texDetailAbd.Sample(sampDetailAbd, float3(matUV, matIndex[i])).rgb), 2.2);
        matNormalMap[i] = texDetailNrm.Sample(sampDetailNrm, float3(matUV, matIndex[i]));
    }

    uint3 dimensions;
    texSplatMap.GetDimensions(0, dimensions.x, dimensions.y, dimensions.z);

    float2 fraction = frac(input.TexCoord * dimensions.xy + 0.5);

    ShaderParams params = CreateShaderParams();

    params.Albedo = lerp(lerp(matAlbedo[3], matAlbedo[2], fraction.x), lerp(matAlbedo[0], matAlbedo[1], fraction.x), fraction.y);
    float4 normalMap = lerp(lerp(matNormalMap[3], matNormalMap[2], fraction.x), lerp(matNormalMap[0], matNormalMap[1], fraction.x), fraction.y);

    params.Roughness = max(0.01, 1.0 - normalMap.z);
    params.AmbientOcclusion = normalMap.w; 

    params.NormalMap = normalMap.xy * 2.0 - 1.0;

    params.Normal = texNrm.Sample(sampNrm, input.TexCoord).xyz * 2.0 - 1.0;
    float3 tangent = normalize(cross(params.Normal, float3(0, 0, 1)));
    float3 binormal = normalize(cross(params.Normal, tangent));

    params.Normal = normalize(
        params.NormalMap.x * tangent + 
        params.NormalMap.y * binormal + 
        sqrt(1.0 - saturate(dot(params.NormalMap.xy, params.NormalMap.xy))) * params.Normal);

    float4 ibl = g_DefaultIBLTexture.SampleLevel(g_LinearClampSampler, params.Normal, mrgDefaultIBLLodParam);
    params.IndirectDiffuse = (mrgDefaultIBLExposurePacked ? UnpackHDR(ibl) : ibl.rgb) * g_DefaultIBLIntensity;

    StoreParams(params, gBuffer0, gBuffer1, gBuffer2, gBuffer3);
}