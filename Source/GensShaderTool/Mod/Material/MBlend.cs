namespace GensShaderTool.Mod.Material;

public class MBlend : Blend
{
    public override string Name => "MBlend";

    public override IReadOnlyList<ShaderParameter> Vectors => Array.Empty<ShaderParameter>();

    public override IReadOnlyList<D3DShaderMacro> Macros => new[]
    {
        new D3DShaderMacro("HasExplicitMetalness", "true")
    };

    public override bool ValidateSamplers(BlendSamplers samplers)
    {
        return samplers.HasFlag(BlendSamplers.Specular) && base.ValidateSamplers(samplers); // Always have specular
    }
}