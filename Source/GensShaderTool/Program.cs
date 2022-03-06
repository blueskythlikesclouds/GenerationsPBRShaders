using GensShaderTool.Mod;
using GensShaderTool.Mod.Deferred;
using GensShaderTool.Mod.Filter;
using GensShaderTool.Mod.Material;

(string, IShader) Shader<T>(string filePath) where T : IShader, new()
{
    return (Path.Combine(@"D:\Repositories\GenerationsPBRShaders\Source\GensShaderTool\Mod", filePath + ".hlsl"), ShaderHandle<T>.Reference);
}

var archiveDatabase = new ArchiveDatabase();

ShaderCompiler.Compile(archiveDatabase, new[]
{
    Shader<DefaultVS>("DefaultVS"),

    Shader<SkyHDR>("Sky"),
    Shader<SkySDR>("Sky"),

    Shader<Blend>("Material/Blend"),
    Shader<MBlend>("Material/Blend"),

    Shader<Common>("Material/Common"),
    Shader<MCommon>("Material/Common"),

    Shader<Emission>("Material/Common"),
    Shader<MEmission>("Material/Common"),

    Shader<CharacterEyeCDRF>("Material/CharacterEye"),
    Shader<CharacterEyeSuper>("Material/CharacterEye"),

    Shader<Character>("Material/Character"),
    Shader<MCharacter>("Material/Character"),

    Shader<CharacterEmission>("Material/Character"),
    Shader<MCharacterEmission>("Material/Character"),

    Shader<CharacterSkinCDRF>("Material/Character"),

    Shader<IgnoreLight>("Material/IgnoreLight"),

    Shader<Ring>("Material/Ring"),

    Shader<SuperSonic>("Material/SuperSonic"),

    Shader<Water01>("Material/Water01"),
    Shader<Water05>("Material/Water05"),
    
    Shader<IBL>("Deferred/IBL"),
    Shader<Light>("Deferred/Light"),

    Shader<BoxBlur>("Filter/BoxBlur"),

    Shader<LUT>("Filter/LUT"),

    Shader<RLR>("Filter/RLR"),

    Shader<VolumetricLighting>("Filter/VolumetricLighting"),
    Shader<VolumetricLightingIgnoreSky>("Filter/VolumetricLighting"),
});

archiveDatabase.Save(@"D:\Steam\steamapps\common\Sonic Generations\mods\PBR Shaders\disk\bb3\shader_pbr.ar.00");