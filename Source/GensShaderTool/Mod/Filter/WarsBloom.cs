namespace GensShaderTool.Mod.Filter;

public class WarsBloom : D3D11PostEffectShader
{
    public override string Name => "PBR_Bloom_BrightPassHDR";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[]
    {
        new ShaderParameter("g_BloomStar_Param1", 151)
    };
}