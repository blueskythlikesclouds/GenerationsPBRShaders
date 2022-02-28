namespace GensShaderTool.Shaders;

public abstract class Shader
{
    public abstract string Name { get; }

    public virtual string EntryPoint => "main";
    public abstract string Target { get; }

    public abstract string Extension { get; }
    public abstract string CodeExtension { get; }
    public abstract string ParameterExtension { get; }

    public virtual IReadOnlyList<ShaderParameter> Vectors => Array.Empty<ShaderParameter>();
    public virtual IReadOnlyList<ShaderParameter> Integers => Array.Empty<ShaderParameter>();
    public virtual IReadOnlyList<ShaderParameter> Booleans => Array.Empty<ShaderParameter>();

    public virtual IReadOnlyList<string> Features => Array.Empty<string>();
    public virtual IReadOnlyList<string> Permutations => Array.Empty<string>();

    public virtual IReadOnlyList<D3DShaderMacro> Macros => Array.Empty<D3DShaderMacro>();

    public virtual bool ValidatePermutation(int features, string permutation)
    {
        return true;
    }

    public ShaderFeaturePair GetFeaturePair(params string[] features)
    {
        var featured = new ShaderFeaturePair(this);

        foreach (var feature in features)
        {
            for (int i = 0; i < features.Length; i++)
            {
                if (feature != Features[i])
                    continue;

                featured.Features |= 1 << i;
                break;
            }
        }

        return featured;
    }
}