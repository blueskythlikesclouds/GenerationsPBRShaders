namespace GensShaderTool.Mod.Material;

[Flags]
public enum Water01Samplers
{
    Diffuse = 1 << 0,
    Normal = 1 << 1,
    Normal1 = 1 << 2,
}

public class Water01 : DefaultPS<DefaultPSFeatures, Water01Samplers>
{
    public override string Name => "Water01";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[] { Common.PBRFactor };

    public override IReadOnlyList<Sampler<Water01Samplers>> Samplers { get; } = new Sampler<Water01Samplers>[]
    {
        new(Water01Samplers.Diffuse, 0, "diffuse", string.Empty),
        new(Water01Samplers.Normal, 1, "normal", string.Empty),
        new(Water01Samplers.Normal1, 2, "normal", string.Empty),
    };

    public override bool ValidateSamplers(Water01Samplers samplers)
    {
        return samplers == (Water01Samplers.Diffuse | Water01Samplers.Normal | Water01Samplers.Normal1);
    }

    public override ShaderVariation GetVertexShader(Water01Samplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(DefaultVSFeatures.NormalMapping | DefaultVSFeatures.NoBone, DefaultVSPermutations.None);
    }
}