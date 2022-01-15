using System;
using System.IO;
using System.Linq;
using System.Text;
using Amicitia.IO.Binary;
using Amicitia.IO.Binary.Extensions;
using Amicitia.IO.Streams;

namespace GensShaderTool
{
    internal class BINA
    {
        public static void Write(Stream destination, Action<BinaryObjectWriter> action)
        {
            using var writer = new BinaryObjectWriter(destination, StreamOwnership.Retain, Endianness.Little, Encoding.UTF8);

            // Prepare header
            for (int i = 0; i < 64; i++)
                writer.Write<byte>(0);

            writer.PushOffsetOrigin();
            action(writer);
            writer.Flush();
            writer.PopOffsetOrigin();

            writer.Seek(0, SeekOrigin.End);
            writer.Align(4);

            long offTableOffset = writer.Position;
            {
                long currentOffset = 64;

                foreach (long offset in writer.OffsetHandler.OffsetPositions.Distinct().OrderBy(x => x))
                {
                    long distance = (offset - currentOffset) >> 2;

                    if (distance > 0x3FFF)
                    {
                        writer.Write((byte)(0xC0 | (distance >> 24)));
                        writer.Write((byte)(distance >> 16));
                        writer.Write((byte)(distance >> 8));
                        writer.Write((byte)(distance & 0xFF));
                    }
                    else if (distance > 0x3F)
                    {
                        writer.Write((byte)(0x80 | (distance >> 8)));
                        writer.Write((byte)distance);
                    }
                    else
                    {
                        writer.Write((byte)(0x40 | distance));
                    }

                    currentOffset = offset;
                }
            }
            writer.Align(4);
            long fileEndOffset = writer.Position;

            writer.Seek(0, SeekOrigin.Begin);
            writer.Write(0x4C303032414E4942);
            writer.Write((uint)fileEndOffset);
            writer.Write(1);
            writer.Write(0x41544144);
            writer.Write((uint)(fileEndOffset - 0x10));
            writer.Write((uint)(offTableOffset - 0x40));
            writer.Write(0);
            writer.Write((uint)(fileEndOffset - offTableOffset));
            writer.Write<ushort>(0x18);
        }
    }
}
