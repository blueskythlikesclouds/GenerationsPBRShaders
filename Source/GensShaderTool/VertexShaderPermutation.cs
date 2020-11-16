using Amicitia.IO.Binary;

namespace GensShaderTool
{
    public class VertexShaderPermutation : IBinarySerializable
    {
        public int Flags { get; set; }
        public string Technique { get; set; }
        public string ShaderName { get; set; }

        public void Read( BinaryObjectReader reader )
        {
            Flags = reader.ReadInt32();
            reader.ReadOffset( () => Technique = reader.ReadString( StringBinaryFormat.NullTerminated ) );
            reader.ReadOffset( () => ShaderName = reader.ReadString( StringBinaryFormat.NullTerminated ) );
        }

        public void Write( BinaryObjectWriter writer )
        {
            writer.Write( Flags );
            writer.WriteStringOffset( StringBinaryFormat.NullTerminated, Technique, -1, 1 );
            writer.WriteStringOffset( StringBinaryFormat.NullTerminated, ShaderName, -1, 1 );
        }

        public VertexShaderPermutation( int flags, string technique, string shaderName )
        {
            Flags = flags;
            Technique = technique;
            ShaderName = shaderName;
        }

        public VertexShaderPermutation()
        {
        }
    }
}