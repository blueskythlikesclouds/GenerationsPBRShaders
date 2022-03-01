namespace GensShaderTool.Mod;

[Flags]
public enum DefaultVSFeatures
{
    NoBone = 1 << 0,
    NoVertexColor = 1 << 1,
    EyeNormal = 1 << 2,
    NormalMapping = 1 << 3,
}

public enum DefaultVSPermutation
{
    None,
    Deferred
}

public class DefaultVS : D3D11VertexShader<DefaultVSFeatures, DefaultVSPermutation>
{
    public override string Name => "Default2";

    public override IReadOnlyList<Permutation<DefaultVSPermutation>> Permutations => new Permutation<DefaultVSPermutation>[]
    {
        new(DefaultVSPermutation.None, "none", string.Empty),
        new(DefaultVSPermutation.Deferred, "defferedlight", "d") 
    };

    public override IReadOnlyList<Feature<DefaultVSFeatures>> Features => new Feature<DefaultVSFeatures>[]
    {
        new(DefaultVSFeatures.NoBone, "b"),
        new(DefaultVSFeatures.NoVertexColor, "c"),
        new(DefaultVSFeatures.EyeNormal, "e"),
        new(DefaultVSFeatures.NormalMapping, "n")
    };

    public override bool ValidatePermutation(DefaultVSFeatures features, Permutation<DefaultVSPermutation> permutation)
    {
        // Eye must exist alone as we don't use normals for eyes and require vertex colors.
        if (features.HasFlag(DefaultVSFeatures.EyeNormal) && features != DefaultVSFeatures.EyeNormal)
            return false;

        // Any other combination is fine.
        return true;
    }
}