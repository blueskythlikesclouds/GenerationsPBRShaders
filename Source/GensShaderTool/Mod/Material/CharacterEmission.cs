namespace GensShaderTool.Mod.Material;

public class CharacterEmission : Character
{
    public override string Name => "ChrEmission";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[] { Common.PBRFactor, Common.Luminance, FalloffFactor };

    public override IReadOnlyList<Sampler<CharacterSamplers>> Samplers { get; } =
        new[] { Diffuse, Specular, Normal, Falloff, Emission };

    public override bool ValidateSamplers(CharacterSamplers samplers)
    {
        return samplers.HasFlag(CharacterSamplers.Diffuse) && samplers.HasFlag(CharacterSamplers.Emission); 
    }
}

public class MCharacterEmission : CharacterEmission
{
    public override string Name => "MChrEmission";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[] { Common.Luminance, FalloffFactor };
    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { Common.MetalnessChannelAlpha };

    public override bool ValidateSamplers(CharacterSamplers samplers)
    {
        return samplers.HasFlag(CharacterSamplers.Specular) && base.ValidateSamplers(samplers);
    }
}