namespace GensShaderTool.Mod;

[Flags]
public enum SkyPSFeatures
{
}

[Flags]
public enum SkyPSPermutations
{
    Default = 1 << 0
}

[Flags]
public enum SkyPSSamplers
{
    Diffuse = 1 << 0,
    Transparency = 1 << 1
}

public abstract class SkyPS : D3D11PixelShader<SkyPSFeatures, SkyPSPermutations, SkyPSSamplers>
{
    public override IReadOnlyList<Permutation<SkyPSPermutations>> Permutations { get; } = new Permutation<SkyPSPermutations>[]
    {
        new(SkyPSPermutations.Default, "default", string.Empty),
    };

    public override IReadOnlyList<Sampler<SkyPSSamplers>> Samplers { get; } = new Sampler<SkyPSSamplers>[]
    {
        new(SkyPSSamplers.Diffuse, 0, "diffuse", "d"),
        new(SkyPSSamplers.Transparency, 1, "transparency", "a"),
    };

    public override bool ValidateSamplers(SkyPSSamplers samplers)
    {
        return samplers.HasFlag(SkyPSSamplers.Diffuse);
    }
}

public class SkyHDR : SkyPS
{
    public override string Name => "Sky2";

    public override ShaderVariation GetVertexShader(SkyPSSamplers samplers, SkyPSFeatures features, Permutation<SkyPSPermutations> permutation)
    {
        return ShaderHandle<SkyVS>.Reference.GetPair(default, SkyVSPermutations.None);
    }
}

public class SkySDR : SkyPS
{
    public override string Name => "Sky3";

    public override IReadOnlyList<D3DShaderMacro> Macros { get; } = new[] { new D3DShaderMacro("IsSDR") };

    public override ShaderVariation GetVertexShader(SkyPSSamplers samplers, SkyPSFeatures features, Permutation<SkyPSPermutations> permutation)
    {
        return ShaderHandle<SkyVS>.Reference.GetPair(SkyVSFeatures.VertexColor | SkyVSFeatures.UvAnimation, SkyVSPermutations.None);
    }
}