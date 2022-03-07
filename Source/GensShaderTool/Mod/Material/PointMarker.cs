namespace GensShaderTool.Mod.Material;

[Flags]
public enum PointMarkerSamplers
{
}

public class PointMarker : DefaultPS<DefaultPSFeatures, PointMarkerSamplers>
{
    public override string Name => "PointMarker";

    public override ShaderVariation GetVertexShader(PointMarkerSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(
            permutation.EnumValue == DefaultPSPermutations.Deferred ? DefaultVSFeatures.Deferred : default, 
            DefaultVSPermutations.None);
    }
}