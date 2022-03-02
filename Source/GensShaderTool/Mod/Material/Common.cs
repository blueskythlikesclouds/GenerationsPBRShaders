namespace GensShaderTool.Mod.Material;

[Flags]
public enum CommonSamplers
{
    Diffuse = 1 << 0,
    Specular = 1 << 1,
    Normal = 1 << 2,
    Transparency = 1 << 3
}

public class Common : DefaultPS<DefaultPSFeatures, CommonSamplers>
{
    public override string Name => "Common2";

    public override IReadOnlyList<ShaderParameter> Vectors => new[]
    {
        new ShaderParameter("PBRFactor", 150)
    };

    public override IReadOnlyList<D3DShaderMacro> Macros => new[]
    {
        new D3DShaderMacro("HasExplicitMetalness", "false")
    };

    public override IReadOnlyList<Sampler<CommonSamplers>> Samplers => new Sampler<CommonSamplers>[]
    {
        new(CommonSamplers.Diffuse, 0, "diffuse", "d"),
        new(CommonSamplers.Specular, 1, "specular", "p"),
        new(CommonSamplers.Normal, 2, "normal", "n"),
        new(CommonSamplers.Transparency, 3, "transparency", "a"),
    };

    public override bool ValidateSamplers(CommonSamplers samplers)
    {
        return samplers.HasFlag(CommonSamplers.Diffuse); // Always have diffuse
    }

    public override ShaderVariation GetVertexShader(CommonSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(

            samplers.HasFlag(CommonSamplers.Normal)
                ? DefaultVSFeatures.NormalMapping :
                default,

            permutation.EnumValue == DefaultPSPermutations.Deferred
                ? DefaultVSPermutations.Deferred
                : DefaultVSPermutations.None);
    }
}