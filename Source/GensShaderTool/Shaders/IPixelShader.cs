namespace GensShaderTool.Shaders;

public interface IPixelShader : IShader
{
    IReadOnlyList<ISampler> Samplers { get; }

    bool ValidateSamplers(int samplers);
    ShaderFeaturePair GetVertexShader(int samplers, int features, IPermutation permutation);
}