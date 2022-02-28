namespace GensShaderTool.Shaders;

public abstract class PixelShader : Shader
{
    public override string Target => "ps_3_0";

    public override string Extension => ".pixelshader";
    public override string CodeExtension => ".wpu";
    public override string ParameterExtension => ".psparam";

    public virtual IReadOnlyList<Sampler> Samplers => Array.Empty<Sampler>();

    public virtual bool ValidateSamplers(int samplers)
    {
        return true;
    }

    public virtual ShaderFeaturePair GetVertexShader(int samplers, int features, string permutation)
    {
        return ShaderFeaturePair.Invalid;
    }
}   