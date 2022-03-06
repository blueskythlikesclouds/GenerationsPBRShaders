namespace GensShaderTool.Compilers.Interop;

public class D3DIncludeCache : IDisposable
{
    private readonly ConcurrentDictionary<string, D3DIncludeHandle> mCache = new(StringComparer.OrdinalIgnoreCase);

    public unsafe void Get(string fullPath, out D3DIncludeHandle handle)
    {
        if (mCache.TryGetValue(fullPath, out handle))
            return;

        using var fileStream = File.OpenRead(fullPath);
        handle.Bytes = (int)fileStream.Length;
        handle.Data = Marshal.AllocHGlobal(handle.Bytes);
        handle.Bytes = fileStream.Read(new Span<byte>(handle.Data.ToPointer(), handle.Bytes));

        if (mCache.TryAdd(fullPath, handle))
            return;

        // If the add fails, this means another thread was quicker than us.
        Marshal.FreeHGlobal(handle.Data);

        // Return the data added by the winning thread.
        if (!mCache.TryGetValue(fullPath, out handle))
            throw new Exception("TryGetValue return value is somehow false!");
    }

    private void ReleaseUnmanagedResources()
    {
        foreach (var handle in mCache.Values)
            Marshal.FreeHGlobal(handle.Data);
    }

    public void Dispose()
    {
        ReleaseUnmanagedResources();
        GC.SuppressFinalize(this);
    }

    ~D3DIncludeCache()
    {
        ReleaseUnmanagedResources();
    }
}