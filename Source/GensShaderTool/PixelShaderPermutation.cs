using System;
using System.Collections.Generic;
using Amicitia.IO.Binary;

namespace GensShaderTool
{
    [Flags]
    public enum PixelShaderSubPermutations
    {
        None = 1 << 0,
        ConstTexCoord = 1 << 1,
        NoGI = 1 << 2,
        NoGI_ConstTexCoord = 1 << 3,
        NoLight = 1 << 4,
        NoLight_ConstTexCoord = 1 << 5,
        NoLight_NoGI = 1 << 6,
        NoLight_NoGI_ConstTexCoord = 1 << 7,
        All = 0xFF
    }

    public class PixelShaderPermutation : IBinarySerializable
    {
        public PixelShaderSubPermutations SubPermutations { get; set; }
        public string Technique { get; set; }
        public string ShaderName { get; set; }
        public List<VertexShaderPermutation> VertexShaderPermutations { get; }

        public void Read( BinaryObjectReader reader )
        {
            SubPermutations = ( PixelShaderSubPermutations ) reader.ReadUInt32();
            reader.ReadOffset( () => Technique = reader.ReadString( StringBinaryFormat.NullTerminated ) );
            reader.ReadOffset( () => ShaderName = reader.ReadString( StringBinaryFormat.NullTerminated ) );

            int count = reader.ReadInt32();
            reader.ReadOffset( () =>
            {
                for ( int i = 0; i < count; i++ )
                    VertexShaderPermutations.Add( reader.ReadObjectOffset<VertexShaderPermutation>() );
            } );
        }

        public void Write( BinaryObjectWriter writer )
        {
            writer.Write( ( uint ) SubPermutations);
            writer.WriteStringOffset( StringBinaryFormat.NullTerminated, Technique, -1, 1 );
            writer.WriteStringOffset( StringBinaryFormat.NullTerminated, ShaderName, -1, 1 );
            writer.Write( VertexShaderPermutations.Count );
            writer.WriteOffset( () =>
            {
                foreach ( var vertexShaderPermutation in VertexShaderPermutations )
                    writer.WriteObjectOffset( vertexShaderPermutation, 4 );
            }, 4 );
        }

        public PixelShaderPermutation()
        {
            VertexShaderPermutations = new List<VertexShaderPermutation>();
        }
    }
}