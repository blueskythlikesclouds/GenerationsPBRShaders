namespace GensShaderTool.Mod.Material;

[Flags]
public enum RingSamplers
{
    Diffuse = 1 << 0,
    Emission = 1 << 1,
    Specular = 1 << 2,
    Normal = 1 << 3,
}

public class Ring : DefaultPS<DefaultPSFeatures, RingSamplers>
{
    public override string Name => "Ring2";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[]
    {
        new ShaderParameter("PBRFactor", 150),
        new ShaderParameter("Blend", 151),
    };

    public override IReadOnlyList<Sampler<RingSamplers>> Samplers { get; } = new Sampler<RingSamplers>[]
    {
        new(RingSamplers.Diffuse, 0, "diffuse", "d"),
        new(RingSamplers.Emission, 1, "emission", "d"),
        new(RingSamplers.Specular, 2, "specular", "p"),
        new(RingSamplers.Normal, 3, "normal", "n"),
    };

    public override bool ValidateSamplers(RingSamplers samplers)
    {
        return
            samplers.HasFlag(RingSamplers.Diffuse) &&
            samplers.HasFlag(RingSamplers.Emission);
    }

    public override ShaderVariation GetVertexShader(RingSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(
            (permutation.EnumValue == DefaultPSPermutations.Deferred ? DefaultVSFeatures.Deferred : default) |
            (samplers.HasFlag(RingSamplers.Normal) ? DefaultVSFeatures.NormalMapping : default),
            DefaultVSPermutations.None);
    }
}