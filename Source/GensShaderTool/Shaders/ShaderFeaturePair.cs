namespace GensShaderTool.Shaders;

public struct ShaderFeaturePair
{
    public IShader Shader;
    public int Features;

    public static readonly ShaderFeaturePair Invalid = default;

    public ShaderFeaturePair(IShader shader, int features)
    {
        Shader = shader;
        Features = features;
    }

    public ShaderFeaturePair(IShader shader) : this(shader, 0)
    {
    }
}