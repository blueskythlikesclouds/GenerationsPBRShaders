namespace GensShaderTool.Shaders;

public class Sampler
{
    public byte Index { get; set; }
    public string Name { get; set; }
    public string Token { get; }

    public Sampler(byte index, string name, string token)
    {
        Index = index;
        Name = name;
        Token = token;
    }
}