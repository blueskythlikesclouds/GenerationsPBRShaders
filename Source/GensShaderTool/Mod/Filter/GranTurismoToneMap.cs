namespace GensShaderTool.Mod.Filter;

public class GranTurismoToneMap : D3D11PostEffectShader
{
    public override string Name => "ToneMap_GranTurismo";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new ShaderParameter[]
    {
        new("g_MiddleGray_Scale_LuminanceLow_LuminanceHigh", 150)
    };
}