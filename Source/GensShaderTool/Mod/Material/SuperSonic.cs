namespace GensShaderTool.Mod.Material;

[Flags]
public enum SuperSonicSamplers
{
    Diffuse = 1 << 0,
    Emission = 1 << 1,
    Specular = 1 << 2,
    Normal = 1 << 3,
    Falloff = 1 << 4
}

public class SuperSonic : DefaultPS<DefaultPSFeatures, SuperSonicSamplers>
{
    public override string Name => "SuperSonic";

    public override IReadOnlyList<ShaderParameter> Vectors => new[]
    {
        new ShaderParameter("Blend", 150),
        new ShaderParameter("FalloffFactor", 151),
    };

    public override IReadOnlyList<Sampler<SuperSonicSamplers>> Samplers => new Sampler<SuperSonicSamplers>[]
    {
        new(SuperSonicSamplers.Diffuse, 0, "diffuse", "d"),
        new(SuperSonicSamplers.Emission, 1, "diffuse", "d"),
        new(SuperSonicSamplers.Specular, 2, "specular", "p"),
        new(SuperSonicSamplers.Normal, 3, "normal", "n"),
        new(SuperSonicSamplers.Falloff, 4, "falloff", "f"),
    };

    public override bool ValidateSamplers(SuperSonicSamplers samplers)
    {
        // Always have diffuse, emission and specular
        return 
            samplers.HasFlag(SuperSonicSamplers.Diffuse) &&  
            samplers.HasFlag(SuperSonicSamplers.Emission) &&  
            samplers.HasFlag(SuperSonicSamplers.Specular);  
    }

    public override ShaderVariation GetVertexShader(SuperSonicSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(

            samplers.HasFlag(SuperSonicSamplers.Normal)
                ? DefaultVSFeatures.NormalMapping :
                default,

            permutation.EnumValue == DefaultPSPermutations.Deferred
                ? DefaultVSPermutations.Deferred
                : DefaultVSPermutations.None);
    }
}