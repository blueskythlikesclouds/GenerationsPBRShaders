namespace GensShaderTool.Shaders;

public abstract class PixelShader<TFeatures, TPermutations, TSamplers> : Shader<TFeatures, TPermutations>, IPixelShader
    where TFeatures : Enum
    where TPermutations : Enum
    where TSamplers : Enum
{
    public override string Target => "ps_3_0";

    public override string Extension => ".pixelshader";
    public override string CodeExtension => ".wpu";
    public override string ParameterExtension => ".psparam";

    public virtual IReadOnlyList<Sampler<TSamplers>> Samplers => Array.Empty<Sampler<TSamplers>>();

    public virtual bool ValidateSamplers(TSamplers samplers)
    {
        return true;
    }

    public virtual ShaderVariation GetVertexShader(TSamplers samplers, TFeatures features, Permutation<TPermutations> permutation)
    {
        return ShaderVariation.Invalid;
    }

    protected TSamplers ConvertSamplers(int samplers)
    {
        return (TSamplers)Enum.ToObject(typeof(TFeatures), samplers);
    }

    IReadOnlyList<ISampler> IPixelShader.Samplers => Samplers;

    bool IPixelShader.ValidateSamplers(int samplers) =>
        ValidateSamplers(ConvertSamplers(samplers));

    ShaderVariation IPixelShader.GetVertexShader(int samplers, int features, IPermutation permutation) =>
        GetVertexShader(ConvertSamplers(samplers), ConvertFeatures(features), ConvertPermutation(permutation));
}   