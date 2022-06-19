namespace GensShaderTool.Mod.Material;

[Flags]
public enum CharacterSamplers
{
    Diffuse = 1 << 0,
    Emission = 1 << 1,
    Specular = 1 << 2,
    Normal = 1 << 3,
    Cdr = 1 << 4,
    Falloff = 1 << 5
}

public class Character : DefaultPS<DefaultPSFeatures, CharacterSamplers>
{
    public override string Name => "Falloff2";

    public static readonly ShaderParameter FalloffFactor = new("FalloffFactor", 152);

    public static readonly Sampler<CharacterSamplers> Diffuse = new(CharacterSamplers.Diffuse, 0, "diffuse", "d");
    public static readonly Sampler<CharacterSamplers> Emission = new(CharacterSamplers.Emission, 1, "emission", "E");
    public static readonly Sampler<CharacterSamplers> Specular = new(CharacterSamplers.Specular, 2, "specular", "p");
    public static readonly Sampler<CharacterSamplers> Normal = new(CharacterSamplers.Normal, 3, "normal", "n");
    public static readonly Sampler<CharacterSamplers> Cdr = new(CharacterSamplers.Cdr, 4, "cdr", "c");
    public static readonly Sampler<CharacterSamplers> Falloff = new(CharacterSamplers.Falloff, 5, "falloff", "f");

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[] { Common.PBRFactor, FalloffFactor };
    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { Common.MetalnessChannelNone };

    public override IReadOnlyList<Sampler<CharacterSamplers>> Samplers { get; } = new[] { Diffuse, Specular, Normal, Falloff };

    public override bool ValidateSamplers(CharacterSamplers samplers)
    {
        return samplers.HasFlag(CharacterSamplers.Diffuse) && 
               samplers.HasFlag(CharacterSamplers.Falloff);
    }

    public override ShaderVariation GetVertexShader(CharacterSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(
            (permutation.EnumValue == DefaultPSPermutations.Deferred ? DefaultVSFeatures.Deferred : default) |
            (samplers.HasFlag(CharacterSamplers.Normal) ? DefaultVSFeatures.NormalMapping : default),
            DefaultVSPermutations.None);
    }
}

public class MCharacter : Character
{
    public override string Name => "MFalloff";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[] { FalloffFactor };
    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { Common.MetalnessChannelAlpha };

    public override bool ValidateSamplers(CharacterSamplers samplers)
    {
        return samplers.HasFlag(CharacterSamplers.Specular) && base.ValidateSamplers(samplers);
    }
}