﻿namespace GensShaderTool.Mod.Material;

public class VertexColorFadeToInvAlpha : DefaultPS<DefaultPSFeatures, CommonSamplers>
{
    public override string Name => "VertexColorFadeToInvAlpha2";

    public override IReadOnlyList<Sampler<CommonSamplers>> Samplers { get; } = new[] { Common.Diffuse };

    public override bool ValidateSamplers(CommonSamplers samplers)
    {
        return samplers.HasFlag(CommonSamplers.Diffuse);
    }

    public override ShaderVariation GetVertexShader(CommonSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(
            permutation.EnumValue == DefaultPSPermutations.Deferred ? DefaultVSFeatures.Deferred : default, DefaultVSPermutations.None);
    }
}