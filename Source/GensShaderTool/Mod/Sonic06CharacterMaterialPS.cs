
namespace GensShaderTool.Mod;

[Flags]
public enum Sonic06CharacterMaterialPSFeatures
{
}

[Flags]
public enum Sonic06CharacterMaterialPSPermutations
{
    Default = 1 << 0
}

[Flags]
public enum Sonic06CharacterMaterialPSSamplers
{
    Diffuse = 1 << 0,
    Specular = 1 << 1,
    Normal = 1 << 2,
    Reflection = 1 << 3,
    Reflection1 = 1 << 4,
    Reflection2 = 1 << 5
}

public class Sonic06CharacterMaterialPS
    : PixelShader<Sonic06CharacterMaterialPSFeatures, Sonic06CharacterMaterialPSPermutations, Sonic06CharacterMaterialPSSamplers>
{
    public override string Name => "Sonic06CharacterMaterial";

    public override IReadOnlyList<Permutation<Sonic06CharacterMaterialPSPermutations>> Permutations => new Permutation<Sonic06CharacterMaterialPSPermutations>[]
    {
        new(Sonic06CharacterMaterialPSPermutations.Default, "default", string.Empty)
    };

    public override IReadOnlyList<Sampler<Sonic06CharacterMaterialPSSamplers>> Samplers => new Sampler<Sonic06CharacterMaterialPSSamplers>[]
    {
        new(Sonic06CharacterMaterialPSSamplers.Diffuse, 0, "diffuse", "d"),
        new(Sonic06CharacterMaterialPSSamplers.Specular, 1, "specular", "s"),
        new(Sonic06CharacterMaterialPSSamplers.Normal, 2, "normal", "n"),
        new(Sonic06CharacterMaterialPSSamplers.Reflection, 3, "reflection", "e"),
        new(Sonic06CharacterMaterialPSSamplers.Reflection1, 3, "reflection", "e1"),
        new(Sonic06CharacterMaterialPSSamplers.Reflection2, 3, "reflection", "e2"),
    };

    public override IReadOnlyList<ShaderParameter> Vectors => new[]
    {
        new ShaderParameter("Luminance", 63)
    };

    public override ShaderVariation GetVertexShader(Sonic06CharacterMaterialPSSamplers samplers, Sonic06CharacterMaterialPSFeatures features, Permutation<Sonic06CharacterMaterialPSPermutations> permutation)
    {
        return ShaderHandle<ChrDefaultVS>.Reference.GetPair((samplers.HasFlag(Sonic06CharacterMaterialPSSamplers.Normal) ? ChrDefaultVSFeatures.Normal : default) | ChrDefaultVSFeatures.VertexColor, ChrDefaultVSPermutations.None);
    }

    public override bool ValidateSamplers(Sonic06CharacterMaterialPSSamplers samplers)
    {
        var reflectionSamplers = Sonic06CharacterMaterialPSSamplers.Reflection | Sonic06CharacterMaterialPSSamplers.Reflection1 | Sonic06CharacterMaterialPSSamplers.Reflection2;
        return (samplers & Sonic06CharacterMaterialPSSamplers.Diffuse) != 0 && int.PopCount((int)(samplers & reflectionSamplers)) <= 1;
    }
}
