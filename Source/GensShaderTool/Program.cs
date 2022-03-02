using GensShaderTool.Mod;
using GensShaderTool.Mod.Filter;
using GensShaderTool.Mod.Material;

(string, IShader) Shader<T>(string filePath) where T : IShader, new()
{
    return (Path.Combine(@"D:\Repositories\GenerationsPBRShaders\Source\GensShaderTool\Mod", filePath + ".hlsl"), ShaderHandle<T>.Reference);
}

var archiveDatabase = new ArchiveDatabase(@"D:\Steam\steamapps\common\Sonic Generations\disk\bb3\shader_r.ar.00");

ShaderCompiler.Compile(archiveDatabase, new[]
{
    Shader<DefaultVS>("DefaultVS"),

    Shader<Common>("Material/Common"),
    Shader<MCommon>("Material/Common"),

    Shader<LUT>("Filter/LUT"),
});

archiveDatabase.Save(@"D:\Steam\steamapps\common\Sonic Generations\mods\Sunset Heights\disk\bb3\shader_r.ar.00");