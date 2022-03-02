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
    public override IReadOnlyList<Permutation<DefaultPSPermutations>> Permutations =>
        new Permutation<DefaultPSPermutations>[]
        {
            new(DefaultPSPermutations.Default, "default", string.Empty),
            new(DefaultPSPermutations.Deferred, "defferedlight", "d")
        };
}