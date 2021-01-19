using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace GensShaderTool
{
    public class ShaderParameterConverter
    {
        public static ShaderParameterSet ParseAssemblyComments( IReadOnlyList<string> lines )
        {
            var parameterSet = new ShaderParameterSet();

            for ( int i = 0; i < lines.Count; i++ )
            {
                if ( !lines[ i ].Contains( "Registers:", StringComparison.OrdinalIgnoreCase ) )
                    continue;

                for ( i += 4; i < lines.Count; i++ )
                {
                    var split = lines[ i ].Trim( '/' ).Split( ' ', StringSplitOptions.RemoveEmptyEntries );
                    if ( split.Length != 3 )
                        return parameterSet;

                    var parameter = new ShaderParameter
                    {
                        Name = split[ 0 ],
                        Index = byte.Parse( split[ 1 ].AsSpan().Slice( 1 ) ),
                        Size = byte.Parse( split[ 2 ] )
                    };

                    switch ( split[ 1 ][ 0 ] )
                    {
                        case 'b':
                            parameterSet.BoolParameters.Add( parameter );
                            break;

                        case 'i':
                            parameterSet.IntParameters.Add( parameter );
                            break;

                        case 'c':
                            parameterSet.SingleParameters.Add( parameter );
                            break;

                        case 's':
                            if (!parameter.Name.StartsWith("g_"))
                                parameter.Index |= (byte)(parameter.Index << 4); // index within mrgTexcoordIndex, going to assume it's the same as the register

                            parameterSet.SamplerParameters.Add( parameter );
                            break;

                        default:
                            throw new InvalidDataException( $"Unknown register type: {split[ 1 ][ 0 ]}" );
                    }
                }
            }

            return parameterSet;
        }

        public static void WriteHlslRegisters( ShaderParameterSet parameterSet, TextWriter writer )
        {
            foreach ( var param in parameterSet.SingleParameters.OrderBy( x => x.Index ) )
            {
                int size = param.Size;
                string type = "float4";

                if ( param.Name.Contains( "Mtx", StringComparison.OrdinalIgnoreCase ) )
                {
                    if ( param.Name.Contains( "Palette", StringComparison.OrdinalIgnoreCase ) )
                    {
                        size /= 3;
                        type = "row_major float3x4";
                    }
                    else
                    {
                        size /= 4;
                        type = "row_major float4x4";
                    }
                }

                writer.Write( "{0} {1}", type, param.Name );
                if ( size > 1 )
                    writer.Write( "[{0}]", size );
                writer.WriteLine( " : register(c{0});\n", param.Index );
            }

            foreach ( var param in parameterSet.IntParameters.OrderBy( x => x.Index ) )
            {
                writer.Write( "int4 {0}", param.Name );
                if ( param.Size > 1 )
                    writer.Write( "[{0}]", param.Size );
                writer.WriteLine( " : register(i{0});\n", param.Index );
            }

            foreach ( var param in parameterSet.BoolParameters.OrderBy( x => x.Index ) )
            {
                writer.Write( "bool {0}", param.Name );
                if ( param.Size > 1 )
                    writer.Write( "[{0}]", param.Size );
                writer.WriteLine( " : register(b{0});\n", param.Index );
            }

            foreach ( var param in parameterSet.SamplerParameters.OrderBy( x => x.Index ) )
            {
                writer.Write( "sampler {0}", param.Name );
                if ( param.Size > 1 )
                    writer.Write( "[{0}]", param.Size );
                writer.WriteLine( " : register(s{0});\n", param.Index );
            }
        }
    }
}