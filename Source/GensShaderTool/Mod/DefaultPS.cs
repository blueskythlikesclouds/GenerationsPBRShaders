namespace GensShaderTool.Mod;

[Flags]
public enum DefaultPSFeatures
{

}

public enum DefaultPSPermutation
{
    Default,
    Deferred
}

public abstract class DefaultPS<TFeatures, TSamplers> : D3D11PixelShader<TFeatures, DefaultPSPermutation, TSamplers>
    where TFeatures : Enum
    where TSamplers : Enum
{
    public override IReadOnlyList<Permutation<DefaultPSPermutation>> Permutations =>
        new Permutation<DefaultPSPermutation>[]
        {
            new(DefaultPSPermutation.Default, "default", string.Empty),
            new(DefaultPSPermutation.Deferred, "defferedlight", "d")
        };
}