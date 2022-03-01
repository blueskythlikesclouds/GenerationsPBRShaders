using GensShaderTool.Mod;
using GensShaderTool.Mod.Material;

(string, IShader) Shader<T>(string filePath) where T : IShader, new()
{
    return (Path.Combine(@"D:\Repositories\GenerationsPBRShaders\Source\GensShaderTool\Mod", filePath + ".hlsl"), ShaderHandle<T>.Reference);
}

var archiveDatabase = new ArchiveDatabase();

ShaderCompiler.Compile(archiveDatabase, new[]
{
    Shader<DefaultVS>("DefaultVS"),
    Shader<Common>("Material/Common")
});

archiveDatabase.Save("test.ar.00");