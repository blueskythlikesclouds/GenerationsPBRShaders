using GensShaderTool.Mod.Material;

namespace GensShaderTool.Mod;

[Flags]
public enum HeightMapPSFeatures
{
}

[Flags]
public enum HeightMapPSPermutations
{
    Default = 1 << 0
}

[Flags]
public enum HeightMapPSSamplers
{
    SplatMap = 1 << 0,
    Normal = 1 << 1,
    DetailAlbedo = 1 << 2,
    DetailNormal = 1 << 3
}

public class HeightMapPS : D3D11PixelShader<HeightMapPSFeatures, HeightMapPSPermutations, HeightMapPSSamplers>
{
    public override string Name => "HeightMap";

    public override IReadOnlyList<Permutation<HeightMapPSPermutations>> Permutations { get; } = new Permutation<HeightMapPSPermutations>[]
    {
        new(HeightMapPSPermutations.Default, "default", string.Empty),
    };

    public override IReadOnlyList<Sampler<HeightMapPSSamplers>> Samplers { get; } = new Sampler<HeightMapPSSamplers>[]
    {
        new(HeightMapPSSamplers.SplatMap, 0, "diffuse", string.Empty),
        new(HeightMapPSSamplers.Normal, 1, "normal", string.Empty),
        new(HeightMapPSSamplers.DetailAlbedo, 2, "diffuse", string.Empty),
        new(HeightMapPSSamplers.DetailNormal, 3, "normal", string.Empty)
    };

    public override bool ValidateSamplers(HeightMapPSSamplers samplers)
    {
        return samplers == (HeightMapPSSamplers.SplatMap | HeightMapPSSamplers.Normal | HeightMapPSSamplers.DetailAlbedo | HeightMapPSSamplers.DetailNormal);
    }

    public override ShaderVariation GetVertexShader(HeightMapPSSamplers samplers, HeightMapPSFeatures features, Permutation<HeightMapPSPermutations> permutation)
    {
        return ShaderHandle<HeightMapVS>.Reference.GetPair(default, HeightMapVSPermutations.None);
    }
}