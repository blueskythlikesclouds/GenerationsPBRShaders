using System.Collections.Generic;
using Amicitia.IO.Binary;

namespace GensShaderTool
{
    public class ShaderList : IBinarySerializable
    {
        public string Name { get; set; }
        public List<PixelShaderPermutation> PixelShaderPermutations { get; }

        public void Read( BinaryObjectReader reader )
        {
            int count = reader.ReadInt32();
            reader.ReadOffset( () =>
            {
                for ( int i = 0; i < count; i++ )
                    PixelShaderPermutations.Add( reader.ReadObjectOffset<PixelShaderPermutation>() );
            } );
        }

        public void Write( BinaryObjectWriter writer )
        {
            writer.Write( PixelShaderPermutations.Count );
            writer.WriteOffset( () =>
            {
                foreach ( var pixelShaderPermutation in PixelShaderPermutations )
                    writer.WriteObjectOffset( pixelShaderPermutation, 4 );
            }, 4 );
        }

        public ShaderList()
        {
            PixelShaderPermutations = new List<PixelShaderPermutation>();
        }
    }
}