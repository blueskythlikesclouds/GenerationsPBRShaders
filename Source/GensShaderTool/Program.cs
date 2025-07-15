using GensShaderTool.Mod;

foreach (var file in Directory.EnumerateFiles(@"C:\Program Files (x86)\Steam\steamapps\common\Sonic Lost World\Mods\Test Mod\disk\sonic2013_patch_0\+shader", "*.pixelshader"))
{
    var pixelShader = new ShaderData();
    pixelShader.Load(file);
    pixelShader.CodeName = pixelShader.CodeName.Replace("EnmMetal_dsfe", "ChrSkin_dsaf");
    pixelShader.ParameterNames[1] = "SonicSkinOpacity";
    pixelShader.Save(file);
}
return;

if (args.Length < 2)
{
    Console.WriteLine("ERROR: Missing command line arguments.");
    return;
}

(string, IShader) Shader<T>(string filePath) where T : IShader, new()
{
    return (Path.Combine(args[0], filePath + ".hlsl"), ShaderHandle<T>.Reference);
}

var archiveDatabase = new ArchiveDatabase();

ShaderCompiler.Compile(archiveDatabase, new[]
{
    Shader<Sonic06CharacterMaterialPS>("Sonic06CharacterMaterialPS")
});

foreach (var databaseData in archiveDatabase.Contents)
    File.WriteAllBytes(Path.Combine(args[1], databaseData.Name), databaseData.Data);