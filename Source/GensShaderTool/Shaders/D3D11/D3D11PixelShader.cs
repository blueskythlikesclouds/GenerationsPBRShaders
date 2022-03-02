namespace GensShaderTool.Shaders.D3D11;

public abstract class
    D3D11PixelShader<TFeatures, TPermutations, TSamplers> : PixelShader<TFeatures, TPermutations, TSamplers>
    where TFeatures : Enum
    where TPermutations : Enum
    where TSamplers : Enum
{
    public override string Target => "ps_5_0";
}