namespace GensShaderTool.Mod.Material;

public class CharacterSkinCDRF : Character
{
    public override string Name => "ChrSkinCDRF";

    public override IReadOnlyList<Sampler<CharacterSamplers>> Samplers { get; } =
        new[] { Diffuse, Specular, Normal, Cdr, Falloff };

    public override bool ValidateSamplers(CharacterSamplers samplers)
    {
        return samplers.HasFlag(CharacterSamplers.Diffuse) && 
               samplers.HasFlag(CharacterSamplers.Cdr) &&
               samplers.HasFlag(CharacterSamplers.Falloff); 
    }
}