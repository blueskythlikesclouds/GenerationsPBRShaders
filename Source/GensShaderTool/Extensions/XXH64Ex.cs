using System.Runtime.CompilerServices;
using K4os.Hash.xxHash;

namespace GensShaderTool.Extensions;

public static class XXH64Ex
{
    public static unsafe void Update<T>(this XXH64 xxHash, T value) where T : unmanaged
    {
        xxHash.Update((byte*)Unsafe.AsPointer(ref value), sizeof(T));
    }

    public static void Update(this XXH64 xxHash, string value)
    {
        if (!string.IsNullOrEmpty(value))
            xxHash.Update(MemoryMarshal.Cast<char, byte>(value.AsSpan()));
    }
}