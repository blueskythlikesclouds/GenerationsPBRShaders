namespace GensShaderTool.Mod;

[Flags]
public enum SkyVSFeatures
{
    UvAnimation = 1 << 0,
    VertexColor = 1 << 1,
}

[Flags]
public enum SkyVSPermutations
{
    None = 1 << 0
}

public class SkyVS : D3D11VertexShader<SkyVSFeatures, SkyVSPermutations>
{
    public override string Name => "Sky";

    public override IReadOnlyList<Permutation<SkyVSPermutations>> Permutations { get; } = new Permutation<SkyVSPermutations>[]
    {
        new(SkyVSPermutations.None, "none", string.Empty),
    };

    public override IReadOnlyList<Feature<SkyVSFeatures>> Features { get; } = new Feature<SkyVSFeatures>[]
    {
        new(SkyVSFeatures.UvAnimation, "u"),
        new(SkyVSFeatures.VertexColor, "v")
    };
}