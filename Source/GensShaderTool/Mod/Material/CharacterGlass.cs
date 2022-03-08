namespace GensShaderTool.Mod.Material;

[Flags]
public enum CharacterGlassSamplers
{
    Diffuse = 1 << 0,
    Specular = 1 << 1,
    Normal = 1 << 2,
    Falloff = 1 << 3
}

public class CharacterGlass : DefaultPS<DefaultPSFeatures, CharacterGlassSamplers>
{
    public override string Name => "ChrGlass";

    public override IReadOnlyList<Permutation<DefaultPSPermutations>> Permutations { get; } = new[] { Default };

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[]
    {
        new ShaderParameter("PBRFactor", 150),
        new ShaderParameter("FalloffFactor", 151),
        new ShaderParameter("MaxOpacity", 152),
    };

    public override IReadOnlyList<Sampler<CharacterGlassSamplers>> Samplers { get; } = new Sampler<CharacterGlassSamplers>[]
    {
        new(CharacterGlassSamplers.Diffuse, 0, "diffuse", "d"),
        new(CharacterGlassSamplers.Specular, 1, "specular", "p"),
        new(CharacterGlassSamplers.Normal, 2, "normal", "n"),
        new(CharacterGlassSamplers.Falloff, 3, "falloff", "f")
    };

    public override bool ValidateSamplers(CharacterGlassSamplers samplers)
    {
        return samplers.HasFlag(CharacterGlassSamplers.Diffuse);
    }

    public override ShaderVariation GetVertexShader(CharacterGlassSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(
            samplers.HasFlag(CharacterGlassSamplers.Normal) ? DefaultVSFeatures.NormalMapping : default,
            DefaultVSPermutations.None);
    }
}