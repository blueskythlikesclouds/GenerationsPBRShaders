namespace GensShaderTool.Shaders;

public interface IShader
{
    string Name { get; }

    string EntryPoint { get; }
    string Target { get; }

    string Extension { get; }
    string CodeExtension { get; }
    string ParameterExtension { get; }

    IReadOnlyList<ShaderParameter> Vectors { get; }
    IReadOnlyList<ShaderParameter> Integers { get; }
    IReadOnlyList<ShaderParameter> Booleans { get; }

    IReadOnlyList<IFeature> Features { get; }
    IReadOnlyList<IPermutation> Permutations { get; }

    IReadOnlyList<D3DShaderMacro> Macros => Array.Empty<D3DShaderMacro>();

    bool ValidatePermutation(int features, IPermutation permutation);
    ShaderFeaturePair GetFeaturePair(int features);
}