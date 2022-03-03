namespace GensShaderTool.Mod.Material;

[Flags]
public enum SkinSamplers
{
    Diffuse = 1 << 0,
    Specular = 1 << 1,
    Normal = 1 << 2,
    Cdr = 1 << 3,
    Falloff = 1 << 4
}

public class Skin : DefaultPS<DefaultPSFeatures, SkinSamplers>
{
    public override string Name => "ChrSkinCDRF";

    public override IReadOnlyList<ShaderParameter> Vectors => new[]
    {
        new ShaderParameter("PBRFactor", 150),
        new ShaderParameter("FalloffFactor", 151),
    };

    public override IReadOnlyList<Sampler<SkinSamplers>> Samplers => new Sampler<SkinSamplers>[]
    {
        new(SkinSamplers.Diffuse, 0, "diffuse", "d"),
        new(SkinSamplers.Specular, 1, "specular", "p"),
        new(SkinSamplers.Normal, 2, "normal", "n"),
        new(SkinSamplers.Cdr, 3, "cdr", "c"),
        new(SkinSamplers.Falloff, 4, "falloff", "f"),
    };

    public override bool ValidateSamplers(SkinSamplers samplers)
    {
        // Always have diffuse, CDR and falloff
        return 
            samplers.HasFlag(SkinSamplers.Diffuse) &&  
            samplers.HasFlag(SkinSamplers.Cdr) &&  
            samplers.HasFlag(SkinSamplers.Falloff);  
    }

    public override ShaderVariation GetVertexShader(SkinSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(

            samplers.HasFlag(SkinSamplers.Normal)
                ? DefaultVSFeatures.NormalMapping :
                default,

            permutation.EnumValue == DefaultPSPermutations.Deferred
                ? DefaultVSPermutations.Deferred
                : DefaultVSPermutations.None);
    }
}