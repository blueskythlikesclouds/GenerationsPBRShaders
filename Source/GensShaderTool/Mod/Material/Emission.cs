namespace GensShaderTool.Mod.Material;

public class Emission : Common
{
    public override string Name => "Emission";

    public override IReadOnlyList<ShaderParameter> Vectors => new[] { PBRFactor, Luminance };
    public override IReadOnlyList<D3DShaderMacro> Macros => new[] { DisableExplicitMetalness };

    public override IReadOnlyList<Sampler<CommonSamplers>> Samplers =>
        new[] { Diffuse, Specular, Normal, Emission, Transparency };

    public override bool ValidateSamplers(CommonSamplers samplers)
    {
        return samplers.HasFlag(CommonSamplers.Diffuse) && samplers.HasFlag(CommonSamplers.Emission);
    }
}

public class MEmission : Emission
{
    public override string Name => "MEmission";

    public override IReadOnlyList<ShaderParameter> Vectors => new[] { Luminance };
    public override IReadOnlyList<D3DShaderMacro> Macros => new[] { EnableExplicitMetalness };

    public override bool ValidateSamplers(CommonSamplers samplers)
    {
        return samplers.HasFlag(CommonSamplers.Specular) && base.ValidateSamplers(samplers); 
    }
}