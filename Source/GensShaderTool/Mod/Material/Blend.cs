namespace GensShaderTool.Mod.Material;

[Flags]
public enum BlendSamplers
{
    Diffuse = 1 << 0,
    Specular = 1 << 1,
    Normal = 1 << 2,
    Blend = 1 << 3,
    DiffuseBlend = 1 << 4,
    SpecularBlend = 1 << 5,
    NormalBlend = 1 << 6
}

public class Blend : DefaultPS<DefaultPSFeatures, BlendSamplers>
{
    public override string Name => "Blend2";

    public override IReadOnlyList<ShaderParameter> Vectors => new[]
    {
        new ShaderParameter("PBRFactor", 150),
        new ShaderParameter("PBRFactor2", 151)
    };

    public override IReadOnlyList<D3DShaderMacro> Macros => new[]
    {
        new D3DShaderMacro("HasExplicitMetalness", "false")
    };

    public override IReadOnlyList<Sampler<BlendSamplers>> Samplers => new Sampler<BlendSamplers>[]
    {
        new(BlendSamplers.Diffuse, 0, "diffuse", "d"),
        new(BlendSamplers.Specular, 1, "specular", "p"),
        new(BlendSamplers.Normal, 2, "normal", "n"),
        new(BlendSamplers.Blend, 3, "opacity", "b"),
        new(BlendSamplers.DiffuseBlend, 4, "diffuse", "d"),
        new(BlendSamplers.SpecularBlend, 5, "specular", "p"),
        new(BlendSamplers.NormalBlend, 6, "normal", "n"),
    };

    public override bool ValidateSamplers(BlendSamplers samplers)
    {
        // Always have diffuse
        if (!samplers.HasFlag(BlendSamplers.Diffuse)) 
            return false;

        uint upperBound = BitHelper.Unpack((uint)samplers, 4, 6);

        // Upper bound should always exist
        if (upperBound == 0)
            return false;

        // Upper bound must be the same as the lower bound
        if (((uint)samplers & 0b111) != upperBound)
            return false;

        // Any other combination is fine
        return true;
    }

    public override ShaderVariation GetVertexShader(BlendSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(

            samplers.HasFlag(BlendSamplers.Normal)
                ? DefaultVSFeatures.NormalMapping :
                default,

            permutation.EnumValue == DefaultPSPermutations.Deferred
                ? DefaultVSPermutations.Deferred
                : DefaultVSPermutations.None);
    }
}