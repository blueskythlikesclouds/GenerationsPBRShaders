namespace GensShaderTool.Mod.Material;

public class CharacterEyeCDRF : CharacterEye
{
    public override string Name => "ChrEyeCDRF";

    public override IReadOnlyList<Sampler<CharacterEyeSamplers>> Samplers =>
        new[] { Diffuse, Specular, Cdr, Reflection };

    public override bool ValidateSamplers(CharacterEyeSamplers samplers)
    {
        return samplers == (CharacterEyeSamplers.Diffuse | 
                            CharacterEyeSamplers.Specular | 
                            CharacterEyeSamplers.Cdr | 
                            CharacterEyeSamplers.Reflection);
    }
}