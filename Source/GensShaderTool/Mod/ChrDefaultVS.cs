
namespace GensShaderTool.Mod;

[Flags]
public enum ChrDefaultVSFeatures
{
    Normal = 1 << 0,
    VertexColor = 1 << 1
}

[Flags]
public enum ChrDefaultVSPermutations
{
    None = 1 << 0,
    Morph = 1 << 1
}

public class ChrDefaultVS : VertexShader<ChrDefaultVSFeatures, ChrDefaultVSPermutations>
{
    public override string Name => "ChrDefault";

    public override IReadOnlyList<Feature<ChrDefaultVSFeatures>> Features => new Feature<ChrDefaultVSFeatures>[]
    {
        new(ChrDefaultVSFeatures.Normal, "n"),
        new(ChrDefaultVSFeatures.VertexColor, "v")
    };

    public override IReadOnlyList<Permutation<ChrDefaultVSPermutations>> Permutations => new Permutation<ChrDefaultVSPermutations>[]
    {
        new(ChrDefaultVSPermutations.None, "none", string.Empty),
        new(ChrDefaultVSPermutations.Morph, "p", "p")
    };
}