namespace GensShaderTool.Mod.Material;

[Flags]
public enum CommonSamplers
{
    Diffuse = 1 << 0,
    Specular = 1 << 1,
    Normal = 1 << 2,
    Emission = 1 << 3,
    Transparency = 1 << 4
}

public class Common : DefaultPS<DefaultPSFeatures, CommonSamplers>
{
    public override string Name => "Common2";

    public static readonly ShaderParameter PBRFactor = new("PBRFactor", 150);
    public static readonly ShaderParameter Luminance = new("Luminance", 151);

    public static readonly D3DShaderMacro MetalnessChannelNone = new("MetalnessChannel", "METALNESS_CHANNEL_NONE"); 
    public static readonly D3DShaderMacro MetalnessChannelBlue = new("MetalnessChannel", "METALNESS_CHANNEL_BLUE"); 
    public static readonly D3DShaderMacro MetalnessChannelAlpha = new("MetalnessChannel", "METALNESS_CHANNEL_ALPHA");

    public static readonly Sampler<CommonSamplers> Diffuse = new(CommonSamplers.Diffuse, 0, "diffuse", "d");
    public static readonly Sampler<CommonSamplers> Specular = new(CommonSamplers.Specular, 1, "specular", "p");
    public static readonly Sampler<CommonSamplers> Normal = new(CommonSamplers.Normal, 2, "normal", "n");
    public static readonly Sampler<CommonSamplers> Emission = new(CommonSamplers.Emission, 3, "emission", "E");
    public static readonly Sampler<CommonSamplers> Transparency = new(CommonSamplers.Transparency, 4, "transparency", "a");

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[] { PBRFactor };
    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { MetalnessChannelNone };

    public override IReadOnlyList<Sampler<CommonSamplers>> Samplers { get; } =
        new[] { Diffuse, Specular, Normal, Transparency };

    public override bool ValidateSamplers(CommonSamplers samplers)
    {
        return samplers.HasFlag(CommonSamplers.Diffuse); 
    }

    public override ShaderVariation GetVertexShader(CommonSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(
            (permutation.EnumValue == DefaultPSPermutations.Deferred ? DefaultVSFeatures.Deferred : default) |
            (samplers.HasFlag(CommonSamplers.Normal) ? DefaultVSFeatures.NormalMapping : default),
            DefaultVSPermutations.None);
    }
}

public class MCommon : Common
{
    public override string Name => "MCommon";

    public override IReadOnlyList<ShaderParameter> Vectors => Array.Empty<ShaderParameter>();
    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { MetalnessChannelAlpha };

    public override bool ValidateSamplers(CommonSamplers samplers)
    {
        return samplers.HasFlag(CommonSamplers.Specular) && base.ValidateSamplers(samplers);
    }
}

public class Common3 : MCommon
{
    public override string Name => "Common3";

    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { MetalnessChannelBlue };
}