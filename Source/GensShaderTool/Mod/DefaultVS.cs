namespace GensShaderTool.Mod;

[Flags]
public enum DefaultVSFeatures
{
    NoBone = 1 << 0,
    NoVertexColor = 1 << 1,
    Deferred = 1 << 2,
    EyeNormal = 1 << 3,
    NormalMapping = 1 << 4,
}

[Flags]
public enum DefaultVSPermutations
{
    None = 1 << 0,
}

public class DefaultVS : D3D11VertexShader<DefaultVSFeatures, DefaultVSPermutations>
{
    public override string Name => "Default2";

    public override IReadOnlyList<Permutation<DefaultVSPermutations>> Permutations => new Permutation<DefaultVSPermutations>[]
    {
        new(DefaultVSPermutations.None, "none", string.Empty),
    };

    public override IReadOnlyList<Feature<DefaultVSFeatures>> Features => new Feature<DefaultVSFeatures>[]
    {
        new(DefaultVSFeatures.NoBone, "b"),
        new(DefaultVSFeatures.NoVertexColor, "c"),
        new(DefaultVSFeatures.Deferred, "d"),
        new(DefaultVSFeatures.EyeNormal, "e"),
        new(DefaultVSFeatures.NormalMapping, "n")
    };

    public override bool ValidatePermutation(DefaultVSFeatures features, Permutation<DefaultVSPermutations> permutation)
    {
        features &= ~DefaultVSFeatures.Deferred;

        // Eye must exist alone as we don't use normals for eyes and require vertex colors.
        if (features.HasFlag(DefaultVSFeatures.EyeNormal) && features != DefaultVSFeatures.EyeNormal)
            return false;

        // Any other combination is fine.
        return true;
    }
}