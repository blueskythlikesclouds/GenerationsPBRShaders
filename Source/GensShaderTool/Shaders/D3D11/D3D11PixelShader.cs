namespace GensShaderTool.Shaders.D3D11;

public abstract class
    D3D11PixelShader<TFeatures, TPermutation, TSamplers> : PixelShader<TFeatures, TPermutation, TSamplers>
    where TFeatures : Enum
    where TPermutation : Enum
    where TSamplers : Enum
{
    public override string Target => "ps_5_0";
}