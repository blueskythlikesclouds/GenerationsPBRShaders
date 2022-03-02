namespace GensShaderTool.Shaders;

public struct ShaderVariation
{
    public IShader Shader;
    public int Features;
    public int Permutations;

    public static readonly ShaderVariation Invalid = default;

    public ShaderVariation(IShader shader, int features, int permutations)
    {
        Shader = shader;
        Features = features;
        Permutations = permutations;
    }
}