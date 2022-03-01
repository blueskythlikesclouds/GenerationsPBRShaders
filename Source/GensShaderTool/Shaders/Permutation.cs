namespace GensShaderTool.Shaders;

public class Permutation<T> : Bit<T>, IPermutation where T : Enum
{
    public string Name { get; }
    public string Suffix { get; }

    public Permutation(T value, string name, string suffix) : base(value)
    {
        Name = name;
        Suffix = suffix;
    }
}