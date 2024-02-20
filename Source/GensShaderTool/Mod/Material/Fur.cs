namespace GensShaderTool.Mod.Material;

[Flags]
public enum FurSamplers
{
    Diffuse = 1 << 0,
    Falloff = 1 << 1,
    Specular = 1 << 2,
    Fur = 1 << 3,
    Flow = 1 << 4,
    Normal = 1 << 5
}

public class Fur : DefaultPS<DefaultPSFeatures, FurSamplers>
{
    public override string Name => "Fur";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new ShaderParameter[]
    {
        new("FalloffFactor", 150),
        new("FurParam", 151),
        new("AnisoFactor", 152),
        new("FurParam2", 153)
    };

    public override IReadOnlyList<Sampler<FurSamplers>> Samplers { get; } = new Sampler<FurSamplers>[]
    {
        new(FurSamplers.Diffuse, 0, "diffuse", string.Empty),
        new(FurSamplers.Falloff, 1, "diffuse", string.Empty),
        new(FurSamplers.Specular, 2, "specular", string.Empty),
        new(FurSamplers.Fur, 3, "reflection", string.Empty),
        new(FurSamplers.Flow, 4, "reflection", string.Empty),
        new(FurSamplers.Normal, 5, "normal", string.Empty),
    };

    public override bool ValidateSamplers(FurSamplers samplers)
    {
        return samplers == (FurSamplers.Diffuse | FurSamplers.Falloff | FurSamplers.Specular | 
                            FurSamplers.Fur | FurSamplers.Flow | FurSamplers.Normal);
    }

    public override ShaderVariation GetVertexShader(FurSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(
            DefaultVSFeatures.NoVertexColor | DefaultVSFeatures.NormalMapping |
            (permutation.EnumValue == DefaultPSPermutations.Deferred ? DefaultVSFeatures.Deferred : default), DefaultVSPermutations.None);
    }
}