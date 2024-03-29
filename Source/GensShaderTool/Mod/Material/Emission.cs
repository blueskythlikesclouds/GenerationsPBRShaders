﻿namespace GensShaderTool.Mod.Material;

public class Emission : Common
{
    public override string Name => "Emission";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[] { PBRFactor, Luminance };
    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { MetalnessChannelNone };

    public override IReadOnlyList<Sampler<CommonSamplers>> Samplers { get; } =
        new[] { Diffuse, Specular, Normal, Emission, Transparency };

    public override bool ValidateSamplers(CommonSamplers samplers)
    {
        return samplers.HasFlag(CommonSamplers.Diffuse) && samplers.HasFlag(CommonSamplers.Emission);
    }
}

public class MEmission : Emission
{
    public override string Name => "MEmission";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[] { Luminance };
    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { MetalnessChannelAlpha };

    public override bool ValidateSamplers(CommonSamplers samplers)
    {
        return samplers.HasFlag(CommonSamplers.Specular) && base.ValidateSamplers(samplers); 
    }
}

public class Emission2 : Emission
{
    public override string Name => "Emission2";

    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { Common.MetalnessChannelBlue };
}