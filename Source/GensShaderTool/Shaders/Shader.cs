namespace GensShaderTool.Shaders;

public abstract class Shader<TFeatures, TPermutations> : IShader
    where TFeatures : Enum
    where TPermutations : Enum
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
    public virtual IReadOnlyList<Permutation<TPermutations>> Permutations => Array.Empty<Permutation<TPermutations>>();

    public virtual IReadOnlyList<D3DShaderMacro> Macros => Array.Empty<D3DShaderMacro>();

    public virtual bool ValidatePermutation(TFeatures features, Permutation<TPermutations> permutation)
    {
        return true;
    }

    public ShaderVariation GetPair(TFeatures features, TPermutations permutations)
    {
        return new ShaderVariation(this, Convert.ToInt32(features), Convert.ToInt32(permutations));
    }

    protected static TFeatures ConvertFeatures(int features)
    {
        return (TFeatures)Enum.ToObject(typeof(TFeatures), features);
    }   
    
    protected static TPermutations ConvertPermutations(int permutations)
    {
        return (TPermutations)Enum.ToObject(typeof(TPermutations), permutations);
    }

    protected static Permutation<TPermutations> ConvertPermutation(IPermutation permutation)
    {
        return permutation as Permutation<TPermutations>;
    }

    IReadOnlyList<IFeature> IShader.Features => Features;
    IReadOnlyList<IPermutation> IShader.Permutations => Permutations;

    bool IShader.ValidatePermutation(int features, IPermutation permutation) =>
        ValidatePermutation(ConvertFeatures(features), ConvertPermutation(permutation));

    ShaderVariation IShader.GetVariation(int features, int permutations) =>
        GetPair(ConvertFeatures(features), ConvertPermutations(permutations));
}