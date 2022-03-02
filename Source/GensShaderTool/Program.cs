using GensShaderTool.Mod;
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

    Shader<Common>("Material/Common"),
    Shader<MCommon>("Material/Common"), 
    
    Shader<Blend>("Material/Blend"),
    Shader<MBlend>("Material/Blend"),

    Shader<LUT>("Filter/LUT"),
});

archiveDatabase.Save(@"D:\Steam\steamapps\common\Sonic Generations\mods\PBR Shaders\disk\bb3\shader_pbr.ar.00");