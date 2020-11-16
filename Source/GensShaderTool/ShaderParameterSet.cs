using System.Collections.Generic;
using Amicitia.IO.Binary;

namespace GensShaderTool
{
    public class ShaderParameterSet : IBinarySerializable
    {
        public List<ShaderParameter> SingleParameters { get; }
        public List<ShaderParameter> IntParameters { get; }
        public List<ShaderParameter> BoolParameters { get; }
        public List<ShaderParameter> SamplerParameters { get; }

        public void Read( BinaryObjectReader reader )
        {
            ReadParameters( SingleParameters );
            ReadParameters( IntParameters );
            ReadParameters( BoolParameters );
            ReadParameters( SamplerParameters );

            void ReadParameters( List<ShaderParameter> parameters )
            {
                int count = reader.ReadInt32();
                reader.ReadOffset( () =>
                {
                    for ( int i = 0; i < count; i++ )
                        parameters.Add( reader.ReadObjectOffset<ShaderParameter>() );
                } );
            }
        }

        public void Write( BinaryObjectWriter writer )
        {
            WriteParameters( SingleParameters );
            WriteParameters( IntParameters );
            WriteParameters( BoolParameters );
            WriteParameters( SamplerParameters );
            writer.Write( 0ul );

            void WriteParameters( List<ShaderParameter> parameters )
            {
                writer.Write( parameters.Count );
                writer.WriteOffset( () =>
                {
                    foreach ( var parameter in parameters )
                        writer.WriteObjectOffset( parameter, 4 );
                }, 4 );
            }
        }

        public ShaderParameterSet()
        {
            SingleParameters = new List<ShaderParameter>();
            IntParameters = new List<ShaderParameter>();
            BoolParameters = new List<ShaderParameter>();
            SamplerParameters = new List<ShaderParameter>();
        }
    }
}