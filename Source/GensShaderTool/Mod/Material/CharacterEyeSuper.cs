namespace GensShaderTool.Mod.Material;

public class CharacterEyeSuper : CharacterEye
{
    public override string Name => "ChrEyeSuper";

    public override IReadOnlyList<ShaderParameter> Vectors { get; } = new[]
    {
        ChrEye1,
        ChrEye2,
        ChrEye3,
        Blend,
    };

    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { AddEmission };
}