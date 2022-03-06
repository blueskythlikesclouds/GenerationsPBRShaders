namespace GensShaderTool.Compilers.Interop;

public class D3DIncludeFactory : IDisposable
{
    public D3DIncludeCache Cache { get; } = new();

    public D3DInclude Create(string directoryPath)
    {
        return new D3DInclude(Cache, directoryPath);
    }

    public void Dispose()
    {
        Cache.Dispose();
    }
}