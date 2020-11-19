#ifndef SCENE_MATERIAL_HLSL_INCLUDED
#define SCENE_MATERIAL_HLSL_INCLUDED

struct Material
{
    float3 Albedo;
    float Alpha;

    float FresnelFactor;
    float Roughness;
    float AmbientOcclusion;
    float Metalness;

    float3 Normal;

    float3 IndirectDiffuse;
    float3 IndirectSpecular;
    float Shadow;

    float3 ViewDirection;
    float CosViewDirection;

    float3 ReflectionDirection;
    float CosReflectionDirection;

    float3 F0;
};

Material NewMaterial()
{
    Material material;

    material.Albedo = 0;
    material.Alpha = 0;

    material.FresnelFactor = 0;
    material.Roughness = 0;
    material.AmbientOcclusion = 0;

    material.Normal = 0;

    material.IndirectDiffuse = 0;
    material.IndirectSpecular = 0;
    material.Shadow = 0;

    material.ViewDirection = 0;
    material.CosViewDirection = 0;

    material.ReflectionDirection = 0;
    material.CosReflectionDirection = 0;

    material.F0 = 0;

    return material;
}

#endif