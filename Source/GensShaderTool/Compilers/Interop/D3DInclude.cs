namespace GensShaderTool.Compilers.Interop;

public enum D3DIncludeType
{
    Local,
    System
}

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
delegate int D3DIncludeOpen(IntPtr pThis, D3DIncludeType includeType, IntPtr pFileName, IntPtr pParentData, ref IntPtr ppData, ref int pBytes);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
delegate int D3DIncludeClose(IntPtr pThis, IntPtr pData);

public class D3DInclude : IDisposable
{
    private D3DIncludeOpen mOpen;
    private D3DIncludeClose mClose;
    private GCHandle mHandle;

    private readonly D3DIncludeCache mCache;
    private readonly Stack<string> mDirectoryPaths;

    public IntPtr Pointer { get; }

    public void Dispose()
    {
        mHandle.Free();
        Marshal.FreeHGlobal(Pointer);
    }

    private static D3DInclude GetInclude(IntPtr pThis)
    {
        return (D3DInclude)GCHandle.FromIntPtr(Marshal.ReadIntPtr(pThis, IntPtr.Size * 3)).Target;
    }

    private static int Open(IntPtr pThis, D3DIncludeType includeType, IntPtr pFileName, IntPtr pParentData, ref IntPtr ppData, ref int pBytes)
    {
        var include = GetInclude(pThis);

        string fullPath = Path.GetFullPath(Path.Combine(include.mDirectoryPaths.Peek(), Marshal.PtrToStringAnsi(pFileName)));
        include.mDirectoryPaths.Push(Path.GetDirectoryName(fullPath));
        include.mCache.Get(fullPath, out var handle);
        ppData = handle.Data;
        pBytes = handle.Bytes;

        return 0;
    }

    private static int Close(IntPtr pThis, IntPtr pData)
    {
        GetInclude(pThis).mDirectoryPaths.Pop();
        return 0;
    }

    public D3DInclude(D3DIncludeCache cache, string directoryPath)
    {
        mHandle = GCHandle.Alloc(this);
        mOpen = Open;
        mClose = Close;

        mCache = cache;
        mDirectoryPaths = new Stack<string>();
        mDirectoryPaths.Push(directoryPath);

        // Allocate an unmanaged block for our type.
        // The layout is as follows:
        // - Virtual function table pointer
        // - Pointer to Open
        // - Pointer to Close
        // - GC handle pointer
        Pointer = Marshal.AllocHGlobal(IntPtr.Size * 4);

        Marshal.WriteIntPtr(Pointer, IntPtr.Size * 0, IntPtr.Add(Pointer, IntPtr.Size));
        Marshal.WriteIntPtr(Pointer, IntPtr.Size * 1, Marshal.GetFunctionPointerForDelegate(mOpen));
        Marshal.WriteIntPtr(Pointer, IntPtr.Size * 2, Marshal.GetFunctionPointerForDelegate(mClose));
        Marshal.WriteIntPtr(Pointer, IntPtr.Size * 3, GCHandle.ToIntPtr(mHandle));
    }
}