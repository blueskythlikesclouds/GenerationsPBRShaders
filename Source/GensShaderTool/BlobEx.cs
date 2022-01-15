using System;
using Vortice.Direct3D;

namespace GensShaderTool
{
    public static class BlobEx
    {
        public static unsafe Span<byte> AsSpan(this Blob blob)
        {
            return new Span<byte>(blob.BufferPointer.ToPointer(), blob.BufferSize);
        }
    }
}
