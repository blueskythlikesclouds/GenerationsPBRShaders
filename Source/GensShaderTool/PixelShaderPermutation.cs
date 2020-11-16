using System.Collections.Generic;
using Amicitia.IO.Binary;

namespace GensShaderTool
{
    public class PixelShaderPermutation : IBinarySerializable
    {
        public int Flags { get; set; }
        public string Technique { get; set; }
        public string ShaderName { get; set; }
        public List<VertexShaderPermutation> VertexShaderPermutations { get; }

        public void Read( BinaryObjectReader reader )
        {
            Flags = reader.ReadInt32();
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
            writer.Write( Flags );
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