namespace GensShaderTool.Mod.Material;

[Flags]
public enum CharacterEyeSamplers
{
    Diffuse = 1 << 0,
    Specular = 1 << 1,
    Cdr = 1 << 2,
    Reflection = 1 << 3,
}

public class CharacterEye : DefaultPS<DefaultPSFeatures, CharacterEyeSamplers>
{
    public override string Name => "ChrEye";

    public static readonly ShaderParameter ChrEye1 = new("ChrEye1", 150);
    public static readonly ShaderParameter ChrEye2 = new("ChrEye2", 151);
    public static readonly ShaderParameter ChrEye3 = new("ChrEye3", 152);
    public static readonly ShaderParameter Blend = new("Blend", 153);

    public static readonly D3DShaderMacro AddEmission = new("AddEmission");

    public static Sampler<CharacterEyeSamplers> Diffuse = new(CharacterEyeSamplers.Diffuse, 0, "diffuse", string.Empty);
    public static Sampler<CharacterEyeSamplers> Specular = new(CharacterEyeSamplers.Specular, 1, "specular", string.Empty);
    public static Sampler<CharacterEyeSamplers> Cdr = new(CharacterEyeSamplers.Cdr, 2, "cdr", string.Empty);
    public static Sampler<CharacterEyeSamplers> Reflection = new(CharacterEyeSamplers.Reflection, 3, "reflection", string.Empty);

    public override IReadOnlyList<ShaderParameter> Vectors => new[] { ChrEye1, ChrEye2, ChrEye3 };

    public override IReadOnlyList<Sampler<CharacterEyeSamplers>> Samplers => new[] { Diffuse, Specular, Reflection };

    public override bool ValidateSamplers(CharacterEyeSamplers samplers)
    {
        return samplers == (CharacterEyeSamplers.Diffuse | CharacterEyeSamplers.Specular | CharacterEyeSamplers.Reflection);
    }

    public override ShaderVariation GetVertexShader(CharacterEyeSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(
            DefaultVSFeatures.EyeNormal,

            permutation.EnumValue == DefaultPSPermutations.Deferred
                ? DefaultVSPermutations.Deferred
                : DefaultVSPermutations.None);
    }
}