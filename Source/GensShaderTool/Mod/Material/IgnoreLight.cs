namespace GensShaderTool.Mod.Material;

[Flags]
public enum IgnoreLightSamplers
{
    Diffuse = 1 << 0,
    Emission = 1 << 1,
    EmissionReflection = 1 << 2,
    Transparency = 1 << 3,
    Reflection = 1 << 4
}

public class IgnoreLight : DefaultPS<DefaultPSFeatures, IgnoreLightSamplers>
{
    public override string Name => "IgnoreLight2";

    public override IReadOnlyList<ShaderParameter> Vectors => new[]
    {
        new ShaderParameter("Luminance", 150),
        new ShaderParameter("Offset", 151),
    };

    public override IReadOnlyList<Sampler<IgnoreLightSamplers>> Samplers => new Sampler<IgnoreLightSamplers>[]
    {
        new(IgnoreLightSamplers.Diffuse, 0, "diffuse", "d" ),
        new(IgnoreLightSamplers.Emission, 1, "emission", "E" ),
        new(IgnoreLightSamplers.EmissionReflection, 1, "emission", "E1" ),
        new(IgnoreLightSamplers.Transparency, 2, "transparency", "a" ),
        new(IgnoreLightSamplers.Reflection, 3, "reflection", "o" )
    };

    public override bool ValidateSamplers(IgnoreLightSamplers samplers)
    {
        bool valid = false;
        int samplerBits = (int)samplers;

        valid |= samplerBits == 0b00001; // d
        valid |= samplerBits == 0b01001; // da
        valid |= samplerBits == 0b11001; // dao
        valid |= samplerBits == 0b00011; // dE
        valid |= samplerBits == 0b00010; // E
        valid |= samplerBits == 0b00100; // E1
        valid |= samplerBits == 0b11010; // Eao

        return valid;
    }

    public override ShaderVariation GetVertexShader(IgnoreLightSamplers samplers, DefaultPSFeatures features, Permutation<DefaultPSPermutations> permutation)
    {
        return ShaderHandle<DefaultVS>.Reference.GetPair(default, 
            permutation.EnumValue == DefaultPSPermutations.Deferred
                ? DefaultVSPermutations.Deferred
                : DefaultVSPermutations.None);
    }
}