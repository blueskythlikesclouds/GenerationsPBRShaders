using System.IO;
using System.Linq;
using System.Text;
using Amicitia.IO.Binary;
using Amicitia.IO.Binary.Extensions;
using Amicitia.IO.Streams;

namespace GensShaderTool
{
    public static class BinarySerializableEx
    {
        public static void Load(this IBinarySerializable binarySerializable, BinaryObjectReader reader)
        {
            reader.Seek( 12, SeekOrigin.Begin );
            reader.ReadOffset( () =>
            {
                using var token = reader.WithOffsetOrigin();
                reader.ReadObject( binarySerializable );
            } );
        }

        public static void Load(this IBinarySerializable binarySerializable, Stream stream)
        {
            using var reader = new BinaryObjectReader(stream, StreamOwnership.Retain, Endianness.Big, Encoding.UTF8);
            binarySerializable.Load(reader);
        }

        public static void Load( this IBinarySerializable binarySerializable, string filePath )
        {
            using var stream = File.OpenRead(filePath);
            binarySerializable.Load(stream);
        }

        public static void Save( this IBinarySerializable binarySerializable, BinaryObjectWriter writer )
        {
            writer.Seek( 24, SeekOrigin.Begin );
            writer.PushOffsetOrigin();
            writer.WriteObject( binarySerializable );
            writer.Flush();
            writer.PopOffsetOrigin();
            writer.Seek( 0, SeekOrigin.End );
            writer.Align( 4 );
            long relocPos = writer.Position;
            writer.Write( writer.OffsetHandler.OffsetPositions.Count() );
            foreach ( long position in writer.OffsetHandler.OffsetPositions )
                writer.Write( ( uint ) ( position - 0x18 ) );

            writer.Write( 0 );
            long length = writer.Position;
            writer.Seek( 0, SeekOrigin.Begin );
            writer.Write( ( uint ) length );
            writer.Write( binarySerializable is ShaderList ? 0 : 2 );
            writer.Write( ( uint ) ( relocPos - 0x18 ) );
            writer.Write( 0x18 );
            writer.Write( ( uint ) relocPos );
            writer.Write( ( uint ) ( length - 4 ) );
        }

        public static void Save(this IBinarySerializable binarySerializable, Stream stream)
        {
            using var writer = new BinaryObjectWriter(stream, StreamOwnership.Retain, Endianness.Big, Encoding.UTF8);
            binarySerializable.Save(writer);
        }

        public static void Save( this IBinarySerializable binarySerializable, string filePath )
        {
            using var stream = File.Create(filePath);
            binarySerializable.Save(stream);
        }

        public static byte[] Save(this IBinarySerializable binarySerializable)
        {
            using var stream = new MemoryStream();
            binarySerializable.Save(stream);
            return stream.ToArray();
        }

        public static T Load<T>( params string[] filePaths ) where T : IBinarySerializable, new()
        {
            var obj = new T();
            foreach ( string filePath in filePaths )
                obj.Load( filePath );
            return obj;
        }
    }
}