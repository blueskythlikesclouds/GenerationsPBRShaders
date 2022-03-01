namespace GensShaderTool.Shaders.D3D11;

public abstract class D3D11VertexShader<TFeatures, TPermutation> : VertexShader<TFeatures, TPermutation>
    where TFeatures : Enum
    where TPermutation : Enum
{
    public override string Target => "vs_5_0";
}