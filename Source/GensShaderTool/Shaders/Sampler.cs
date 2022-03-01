namespace GensShaderTool.Shaders;

public class Sampler<T> : Bit<T>, ISampler where T : Enum 
{
    public byte Index { get; }
    public string Unit { get; }
    public string Suffix { get; }

    public Sampler(T value, byte index, string unit, string suffix) : base(value)
    {
        Index = index;
        Unit = unit;
        Suffix = suffix;
    }
}