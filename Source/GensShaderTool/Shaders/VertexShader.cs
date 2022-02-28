namespace GensShaderTool.Shaders;

public abstract class VertexShader : Shader
{
    public override string Target => "vs_3_0";

    public override string Extension => ".vertexshader";
    public override string CodeExtension => ".wvu";
    public override string ParameterExtension => ".vsparam";
}