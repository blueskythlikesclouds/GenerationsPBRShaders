namespace GensShaderTool.Compilers.Interop.Extensions;

public static unsafe class D3DBlobEx
{
    public static ReadOnlySpan<byte> AsSpan(this ID3DBlob blob)
    {
        return new ReadOnlySpan<byte>((void*)blob.GetBufferPointer(), blob.GetBufferSize());
    }

    public static byte[] ToArray(this ID3DBlob blob) 
    {
        return AsSpan(blob).ToArray();
    }

    public static string ConvertToString(this ID3DBlob blob)
    {
        return Marshal.PtrToStringAnsi(blob.GetBufferPointer());
    }
}