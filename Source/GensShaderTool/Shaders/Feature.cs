namespace GensShaderTool.Shaders;

public class Feature<T> : Bit<T>, IFeature where T : Enum
{
    public string Suffix { get; }

    public Feature(T value, string suffix) : base(value)
    {
        Suffix = suffix;
    }
}