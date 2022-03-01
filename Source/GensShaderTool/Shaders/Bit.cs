namespace GensShaderTool.Shaders;

public class Bit<T> : IBit where T : Enum
{
    public T EnumValue { get; }

    public int BitValue => Convert.ToInt32(EnumValue);
    public string BitName => Enum.GetName(typeof(T), EnumValue);

    public Bit(T value)
    {
        EnumValue = value;
    }
}