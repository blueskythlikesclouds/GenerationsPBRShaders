using System.Collections.Generic;
using Amicitia.IO.Binary;

namespace GensShaderTool
{
    public class Shader : IBinarySerializable
    {
        public string FileName { get; set; }
        public List<string> ParameterFileNames { get; }

        public void Read( BinaryObjectReader reader )
        {
            reader.ReadOffset( () => FileName = reader.ReadString( StringBinaryFormat.NullTerminated ) );

            int count = reader.ReadInt32();
            reader.ReadOffset( () =>
            {
                for ( int i = 0; i < count; i++ )
                {
                    string value = string.Empty;
                    reader.ReadOffset( () => value = reader.ReadString( StringBinaryFormat.NullTerminated ) );
                    ParameterFileNames.Add( value );
                }
            } );
        }

        public void Write( BinaryObjectWriter writer )
        {
            writer.WriteStringOffset( StringBinaryFormat.NullTerminated, FileName, -1, 1 );
            writer.Write( ParameterFileNames.Count );
            writer.WriteOffset( () =>
            {
                foreach ( string parameterFileName in ParameterFileNames )
                    writer.WriteStringOffset( StringBinaryFormat.NullTerminated, parameterFileName, -1, 1 );
            }, 4 );
        }

        public Shader()
        {
            ParameterFileNames = new List<string>();
        }
    }
}