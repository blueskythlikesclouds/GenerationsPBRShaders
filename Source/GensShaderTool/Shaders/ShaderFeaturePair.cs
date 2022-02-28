namespace GensShaderTool.Shaders;

public struct ShaderFeaturePair
{
    public Shader Shader;
    public int Features;

    public static readonly ShaderFeaturePair Invalid = default;

    public ShaderFeaturePair(Shader shader, int features)
    {
        Shader = shader;
        Features = features;
    }

    public ShaderFeaturePair(Shader shader) : this(shader, 0)
    {
    }
}