namespace GensShaderTool.Shaders;

public interface ISampler : IBit
{
    byte Index { get; }
    string Unit { get; }
    string Suffix { get; }
}