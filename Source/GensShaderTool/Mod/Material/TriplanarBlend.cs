namespace GensShaderTool.Mod.Material;

[Flags]
public enum TriplanarBlendSamplers
{
    Diffuse = 1 << 0,
    Specular = 1 << 1,
    Normal = 1 << 2,
    DiffuseBlend = 1 << 3,
    SpecularBlend = 1 << 4,
    NormalBlend = 1 << 5,
    NormalDetail = 1 << 6
}

public class TriplanarBlend : DefaultPS<DefaultPSFeatures, TriplanarBlendSamplers>
{
    public override string Name => "TriplanarBlend";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[]
    {
        new ShaderParameter("Scale", 150),
        new ShaderParameter("Scale2", 151)
    };

    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { Common.MetalnessChannelNone };

    public override IReadOnlyList<Sampler<TriplanarBlendSamplers>> Samplers { get; } = new Sampler<TriplanarBlendSamplers>[]
    {
        new(TriplanarBlendSamplers.Diffuse, 0, "diffuse", "d"),
        new(TriplanarBlendSamplers.Specular, 1, "specular", "p"),
        new(TriplanarBlendSamplers.Normal, 2, "normal", "n"),
        new(TriplanarBlendSamplers.DiffuseBlend, 3, "diffuse", "d"),
        new(TriplanarBlendSamplers.SpecularBlend, 4, "specular", "p"),
        new(TriplanarBlendSamplers.NormalBlend, 5, "normal", "n"),
        new(TriplanarBlendSamplers.NormalDetail, 6, "normal", "n"),
    };

    public override bool ValidateSamplers(TriplanarBlendSamplers samplers)
    {
        return
            samplers == (TriplanarBlendSamplers.Diffuse | TriplanarBlendSamplers.Specular | TriplanarBlendSamplers.Normal | 
                         TriplanarBlendSamplers.DiffuseBlend | TriplanarBlendSamplers.SpecularBlend | TriplanarBlendSamplers.NormalBlend) ||

            samplers == (TriplanarBlendSamplers.Diffuse | TriplanarBlendSamplers.Specular | TriplanarBlendSamplers.Normal | 
                         TriplanarBlendSamplers.DiffuseBlend | TriplanarBlendSamplers.SpecularBlend | TriplanarBlendSamplers.NormalBlend |
                         TriplanarBlendSamplers.NormalDetail);
    }

    public override ShaderVariation GetVertexShader(TriplanarBlendSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(
            (permutation.EnumValue == DefaultPSPermutations.Deferred ? DefaultVSFeatures.Deferred : default) |
            (samplers.HasFlag(TriplanarBlendSamplers.Normal) ? DefaultVSFeatures.NormalMapping : default),
            DefaultVSPermutations.None);
    }
}