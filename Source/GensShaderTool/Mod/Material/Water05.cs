namespace GensShaderTool.Mod.Material;

[Flags]
public enum Water05Samplers
{
    Diffuse = 1 << 0,
    Specular = 1 << 1,
    Normal = 1 << 2,
    Normal1 = 1 << 3
}

public class Water05 : DefaultPS<DefaultPSFeatures, Water05Samplers>
{
    public override string Name => "Water05";

    public override IReadOnlyList<Permutation<DefaultPSPermutations>> Permutations { get; } = new[] { Default };

    public override IReadOnlyList<ShaderParameter> Vectors { get; } =
        new[] { new ShaderParameter("RefractionCubemap", 150) };

    public override IReadOnlyList<Sampler<Water05Samplers>> Samplers { get; } = new Sampler<Water05Samplers>[]
    {
        new(Water05Samplers.Diffuse, 0, "diffuse", string.Empty),
        new(Water05Samplers.Specular, 1, "specular", string.Empty),
        new(Water05Samplers.Normal, 2, "normal", string.Empty),
        new(Water05Samplers.Normal1, 3, "normal", string.Empty),
    };

    public override bool ValidateSamplers(Water05Samplers samplers)
    {
        return samplers == (Water05Samplers.Diffuse | Water05Samplers.Specular | Water05Samplers.Normal | Water05Samplers.Normal1);
    }

    public override ShaderVariation GetVertexShader(Water05Samplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(DefaultVSFeatures.NormalMapping | DefaultVSFeatures.NoBone, DefaultVSPermutations.None);
    }
}