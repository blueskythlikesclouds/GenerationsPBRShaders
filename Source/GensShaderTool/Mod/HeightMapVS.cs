namespace GensShaderTool.Mod;

[Flags]
public enum HeightMapVSFeatures
{
}

[Flags]
public enum HeightMapVSPermutations
{
    None = 1 << 0
}

public class HeightMapVS : D3D11VertexShader<HeightMapVSFeatures, HeightMapVSPermutations>
{
    public override string Name => "HeightMap_Default";

    public override IReadOnlyList<Permutation<HeightMapVSPermutations>> Permutations { get; } = new Permutation<HeightMapVSPermutations>[]
    {
        new(HeightMapVSPermutations.None, "none", string.Empty)
    };
}