namespace GensShaderTool.Mod.Material;

public class MCommon : Common
{
    public override string Name => "MCommon";

    public override IReadOnlyList<ShaderParameter> Vectors => Array.Empty<ShaderParameter>();

    public override IReadOnlyList<D3DShaderMacro> Macros => new[]
    {
        new D3DShaderMacro("HasExplicitMetalness", "true")
    };

    public override bool ValidateSamplers(CommonSamplers samplers)
    {
        return samplers.HasFlag(CommonSamplers.Specular) && base.ValidateSamplers(samplers); // Always have specular
    }
}