namespace GensShaderTool.Mod.Filter;

public class VolumetricLighting : D3D11PostEffectShader
{
    public override string Name => "FxVolumetricLighting";
}

public class VolumetricLightingIgnoreSky : D3D11PostEffectShader
{
    public override string Name => "FxVolumetricLighting_IgnoreSky";
    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { new D3DShaderMacro("IgnoreSky") };
}