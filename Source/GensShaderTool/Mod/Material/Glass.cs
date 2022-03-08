namespace GensShaderTool.Mod.Material;

public class Glass : DefaultPS<DefaultPSFeatures, CommonSamplers>
{
    public override string Name => "Glass2";

    public override IReadOnlyList<Permutation<DefaultPSPermutations>> Permutations { get; } = new[] { Default };

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[]
    {
        Common.PBRFactor,
        Common.Luminance,
        Character.FalloffFactor,
        new ShaderParameter("Refraction", 153)
    };

    public override IReadOnlyList<Sampler<CommonSamplers>> Samplers { get; } =
        new[] { Common.Diffuse, Common.Specular, Common.Normal, Common.Emission };

    public override bool ValidateSamplers(CommonSamplers samplers)
    {
        return samplers.HasFlag(CommonSamplers.Diffuse);
    }

    public override ShaderVariation GetVertexShader(CommonSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(
            samplers.HasFlag(CommonSamplers.Normal) ? DefaultVSFeatures.NormalMapping : default,
            DefaultVSPermutations.None);
    }
}