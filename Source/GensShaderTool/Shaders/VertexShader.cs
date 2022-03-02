namespace GensShaderTool.Shaders;

public abstract class VertexShader<TFeatures, TPermutations> : Shader<TFeatures, TPermutations>, IVertexShader
    where TFeatures : Enum
    where TPermutations : Enum
{
    public override string Target => "vs_3_0";

    public override string Extension => ".vertexshader";
    public override string CodeExtension => ".wvu";
    public override string ParameterExtension => ".vsparam";
}