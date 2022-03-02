namespace GensShaderTool.Shaders.D3D11;

public abstract class D3D11VertexShader<TFeatures, TPermutations> : VertexShader<TFeatures, TPermutations>
    where TFeatures : Enum
    where TPermutations : Enum
{
    public override string Target => "vs_5_0";
}