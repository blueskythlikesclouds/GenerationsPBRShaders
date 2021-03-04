using System;
using Amicitia.IO.Binary;

namespace GensShaderTool
{
    [Flags]
    public enum VertexShaderSubPermutations
    {
        None = 1 << 0,
        ConstTexCoord = 1 << 1,
        All = 0x3
    }

    public class VertexShaderPermutation : IBinarySerializable
    {
        public VertexShaderSubPermutations SubPermutations { get; set; }
        public string Technique { get; set; }
        public string ShaderName { get; set; }

        public void Read( BinaryObjectReader reader )
        {
            SubPermutations = ( VertexShaderSubPermutations ) reader.ReadUInt32();
            reader.ReadOffset( () => Technique = reader.ReadString( StringBinaryFormat.NullTerminated ) );
            reader.ReadOffset( () => ShaderName = reader.ReadString( StringBinaryFormat.NullTerminated ) );
        }

        public void Write( BinaryObjectWriter writer )
        {
            writer.Write( ( uint ) SubPermutations );
            writer.WriteStringOffset( StringBinaryFormat.NullTerminated, Technique, -1, 1 );
            writer.WriteStringOffset( StringBinaryFormat.NullTerminated, ShaderName, -1, 1 );
        }

        public VertexShaderPermutation( VertexShaderSubPermutations subPermutations, string technique, string shaderName )
        {
            SubPermutations = subPermutations;
            Technique = technique;
            ShaderName = shaderName;
        }

        public VertexShaderPermutation()
        {
        }
    }
}