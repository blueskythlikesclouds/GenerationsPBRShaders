using GensShaderTool.Mod;
using GensShaderTool.Mod.Deferred;
using GensShaderTool.Mod.Filter;
using GensShaderTool.Mod.Material;
using GensShaderTool.Mod.Vanilla;

if (args.Length < 3)
{
    Console.WriteLine("ERROR: Missing command line arguments.");
    return;
}

if (!File.Exists(Path.Combine(args[1], "shader_vanilla.ar.00")))
{
    var shaderRegular = new ArchiveDatabase(Path.Combine(args[2], "shader_r.ar.00"));
    var shaderRegularAdd = new ArchiveDatabase(Path.Combine(args[2], "shader_r_add.ar.00"));

    void MoveShader(string initials) => 
        shaderRegularAdd.Contents.AddRange(shaderRegular.Contents.Where(x => x.Name.StartsWith(initials)));

    MoveShader("Common_dn");
    MoveShader("Default_");
    MoveShader("IgnoreLight");
    MoveShader("MakeShadowMap");
    MoveShader("Sys");

    shaderRegular.Contents.RemoveAll(x => shaderRegularAdd.Contents.Contains(x));

    var postEffectShaderNames = shaderRegular.Contents
        .Select(x => Path.GetFileNameWithoutExtension(x.Name)).ToHashSet();

    shaderRegular.Contents.AddRange(shaderRegularAdd.Contents.Where(x => postEffectShaderNames.Any(y => x.Name.StartsWith(y))));
    shaderRegularAdd.Contents.RemoveAll(x => shaderRegular.Contents.Contains(x));

    var shaderConverter = new ShaderConverter(args[0]);

    Parallel.ForEach(shaderRegular.Contents.Where(x => x.Name.EndsWith(".wpu") || x.Name.EndsWith(".wvu")), x =>
    {
        Console.WriteLine(x.Name);

        x.Data = shaderConverter.ConvertPostEffectShader(x.Name, x.Data);
        x.Time = DateTime.Now;
    });

    Parallel.ForEach(shaderRegularAdd.Contents.Where(x => x.Name.EndsWith(".wpu") || x.Name.EndsWith(".wvu")), x =>
    {
        Console.WriteLine(x.Name);

        x.Data = shaderConverter.ConvertMaterialShader(x.Name, x.Data);
        x.Time = DateTime.Now;
    });

    shaderRegular.Contents.AddRange(shaderRegularAdd.Contents);

    shaderRegular.Sort();
    shaderRegular.Save(Path.Combine(args[1], "shader_vanilla.ar.00"));
    return;
}

(string, IShader) Shader<T>(string filePath) where T : IShader, new()
{
    return (Path.Combine(args[0], filePath + ".hlsl"), ShaderHandle<T>.Reference);
}

var archiveDatabase = new ArchiveDatabase();

string filePath = Path.Combine(args[1], "shader_pbr.ar.00");
if (File.Exists(filePath))
    archiveDatabase.Load(filePath);

ShaderCompiler.Compile(archiveDatabase, new[]
{
    Shader<DefaultVS>("DefaultVS"),

    Shader<HeightMapVS>("HeightMapVS"),
    Shader<HeightMapPS>("HeightMapPS"),

    Shader<SkyHDR>("Sky"),
    Shader<SkySDR>("Sky"),

    Shader<Blend>("Material/Blend"),
    Shader<MBlend>("Material/Blend"),
    Shader<Blend3>("Material/Blend"),

    Shader<Common>("Material/Common"),
    Shader<MCommon>("Material/Common"),
    Shader<Common3>("Material/Common"),
    
    Shader<Emission>("Material/Common"),
    Shader<MEmission>("Material/Common"),
    Shader<Emission2>("Material/Common"),

    Shader<Glass>("Material/Glass"),

    Shader<CharacterEyeCDRF>("Material/CharacterEye"),
    Shader<CharacterEyeSuper>("Material/CharacterEye"),

    Shader<Character>("Material/Character"),
    Shader<MCharacter>("Material/Character"),

    Shader<CharacterEmission>("Material/Character"),
    Shader<MCharacterEmission>("Material/Character"),

    Shader<CharacterSkinCDRF>("Material/Character"),

    Shader<CharacterGlass>("Material/CharacterGlass"),

    Shader<IgnoreLight>("Material/IgnoreLight"),

    Shader<PointMarker>("Material/PointMarker"),

    Shader<Ring>("Material/Ring"),

    Shader<SuperSonic>("Material/SuperSonic"),

    Shader<Water01>("Material/Water01"),
    Shader<Water05>("Material/Water05"),

    Shader<TriplanarBlend>("Material/TriplanarBlend"),

    Shader<TransThin>("Material/TransThin"),

    Shader<VertexColorFadeToInvAlpha>("Material/VertexColorFadeToInvAlpha"),

    Shader<IBL>("Deferred/IBL"),
    Shader<Light>("Deferred/Light"),

    Shader<BoxBlur>("Filter/BoxBlur"),

    Shader<LUT>("Filter/LUT"),

    Shader<RLR>("Filter/RLR"),

    Shader<VolumetricLighting>("Filter/VolumetricLighting"),
    Shader<VolumetricLightingIgnoreSky>("Filter/VolumetricLighting"),
    
    Shader<WarsBloom>("Filter/WarsBloom"),
    
    Shader<Sonic2010Bloom>("Filter/Sonic2010Bloom"),
    
    Shader<DownSampleN>("Filter/DownSampleN"),

    Shader<BicubicFilter>("Filter/BicubicFilter"),

    Shader<GaussianBlur>("Filter/GaussianBlur"),

    Shader<GranTurismoToneMap>("Filter/GranTurismoToneMap")
});

archiveDatabase.Sort();
archiveDatabase.Save(filePath);