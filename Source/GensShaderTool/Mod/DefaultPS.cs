namespace GensShaderTool.Mod;

[Flags]
public enum DefaultPSFeatures
{

}

[Flags]
public enum DefaultPSPermutations
{
    Default = 1 << 0,
    Deferred = 1 << 1
}

public abstract class DefaultPS<TFeatures, TSamplers> : D3D11PixelShader<TFeatures, DefaultPSPermutations, TSamplers>
    where TFeatures : Enum
    where TSamplers : Enum
{
    public static readonly Permutation<DefaultPSPermutations> Default = new(DefaultPSPermutations.Default, "default", string.Empty);
    public static readonly Permutation<DefaultPSPermutations> Deferred = new(DefaultPSPermutations.Deferred, "deferred", "d");

    public override IReadOnlyList<Permutation<DefaultPSPermutations>> Permutations { get; } =
        new[] { Default, Deferred };
}