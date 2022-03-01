namespace GensShaderTool.Shaders;

public abstract class Shader<TFeatures, TPermutation> : IShader
    where TFeatures : Enum
    where TPermutation : Enum
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

    public virtual IReadOnlyList<Feature<TFeatures>> Features => Array.Empty<Feature<TFeatures>>();
    public virtual IReadOnlyList<Permutation<TPermutation>> Permutations => Array.Empty<Permutation<TPermutation>>();

    public virtual IReadOnlyList<D3DShaderMacro> Macros => Array.Empty<D3DShaderMacro>();

    public virtual bool ValidatePermutation(TFeatures features, Permutation<TPermutation> permutation)
    {
        return true;
    }

    public ShaderFeaturePair GetFeaturePair(TFeatures features)
    {
        return new ShaderFeaturePair(this, Convert.ToInt32(features));
    }

    protected static TFeatures ConvertFeatures(int features)
    {
        return (TFeatures)Enum.ToObject(typeof(TFeatures), features);
    }

    protected static Permutation<TPermutation> ConvertPermutation(IPermutation permutation)
    {
        return permutation as Permutation<TPermutation>;
    }

    IReadOnlyList<IFeature> IShader.Features => Features;
    IReadOnlyList<IPermutation> IShader.Permutations => Permutations;

    bool IShader.ValidatePermutation(int features, IPermutation permutation) =>
        ValidatePermutation(ConvertFeatures(features), ConvertPermutation(permutation));

    ShaderFeaturePair IShader.GetFeaturePair(int features) =>
        GetFeaturePair(ConvertFeatures(features));
}