namespace GensShaderTool.Shaders;

public abstract class PostEffectShader : IShader
{
    public abstract string Name { get; }

    public string EntryPoint => "main";
    public virtual string Target => "ps_3_0";
    public string Extension => ".pixelshader";
    public string CodeExtension => ".wpu";
    public string ParameterExtension => ".psparam";

    public IReadOnlyList<ShaderParameter> Vectors => Array.Empty<ShaderParameter>();
    public IReadOnlyList<ShaderParameter> Integers => Array.Empty<ShaderParameter>();
    public IReadOnlyList<ShaderParameter> Booleans => Array.Empty<ShaderParameter>();
    public IReadOnlyList<IFeature> Features => Array.Empty<IFeature>();
    public IReadOnlyList<IPermutation> Permutations => Array.Empty<IPermutation>();
    public virtual IReadOnlyList<D3DShaderMacro> Macros => Array.Empty<D3DShaderMacro>();

    public bool ValidatePermutation(int features, IPermutation permutation)
    {
        return true;
    }

    public ShaderVariation GetVariation(int features, int permutations)
    {
        return ShaderVariation.Invalid;
    }
}