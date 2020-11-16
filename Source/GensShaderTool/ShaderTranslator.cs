using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace GensShaderTool
{
    public static class ShaderTranslator
    {
        public static void TranslateToShaderModel5( string sourceFilePath, string outputDirectoryPath )
        {
            string asmFilePath = sourceFilePath + ".asm";

            using ( var process = Process.Start( "fxc", $"/dumpbin \"{sourceFilePath}\" /Fc \"{asmFilePath}\"" ) )
            {
                process.WaitForExit();
            }

            var lines = File.ReadAllLines( asmFilePath );

            var semantics = new List<(string, string)>();
            var constants = new Dictionary<string, (string, string, string, string)>();

            ShaderType type = default;

            int i;
            for ( i = 0; i < lines.Length; i++ )
            {
                string prettyLine = lines[ i ].Trim();
                if ( prettyLine.StartsWith( "//" ) || string.IsNullOrEmpty( prettyLine ))
                    continue;

                if ( prettyLine == "vs_3_0" )
                {
                    type = ShaderType.Vertex;
                }

                else if ( prettyLine == "ps_3_0" )
                {
                    type = ShaderType.Pixel;
                }

                else if ( prettyLine.StartsWith( "def" ) )
                {
                    var split = prettyLine.Split( ',' );
                    constants.Add( split[ 0 ].Substring( split[ 0 ].IndexOf( ' ' ) + 1 ),
                        ( split[ 1 ].Trim(), split[ 2 ].Trim(), split[ 3 ].Trim(), split[ 4 ].Trim() ) );
                }

                else if ( prettyLine.StartsWith( "dcl" ) )
                {
                    var split = prettyLine.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
                    var split2 = split[ 0 ].Split( '_' );

                    semantics.Add( ( split2.Length > 1 ? split2[ 1 ].ToUpperInvariant() : "VPOS", split[ 1 ] ) );
                }
                else
                {
                    break;
                }
            }

            string outputFilePath = Path.Combine( outputDirectoryPath, Path.GetFileName( sourceFilePath ) );
            string outputHlslFilePath = outputFilePath + ".hlsl";

            using ( var writer = File.CreateText( outputHlslFilePath ) )
            {
                writer.WriteLine( "cbuffer cbConstants : register(b0)" );
                writer.WriteLine( "{" );
                writer.WriteLine( "\tfloat4 constants[224];" );
                writer.WriteLine( "\tbool booleans[16];" );
                writer.WriteLine( "}" );
                writer.WriteLine();

                foreach ( (string name, string register) in semantics )
                {
                    switch ( name )
                    {
                        case "2D":
                            writer.WriteLine( "Texture2D<float4> t{0} : register(t{0});", register.Substring( 1 ) );
                            writer.WriteLine( "SamplerState {0} : register({0});", register );
                            writer.WriteLine();
                            break;

                        case "CUBE":
                            writer.WriteLine( "TextureCube<float4> t{0} : register(t{0});", register.Substring( 1 ) );
                            writer.WriteLine( "SamplerState {0} : register({0});", register );
                            writer.WriteLine();
                            break;
                    }
                }

                writer.WriteLine( "void main(\n{0},\n\tout float4 oC0 : SV_Target0)\n{{",
                    string.Join( ", \n", semantics.Where( x => x.Item1 != "2D" && x.Item1 != "CUBE" ).Select( x => $"\t{( x.Item2[ 0 ] == 'o' ? "out" : "in" )} float4 {x.Item2.Split( '.' )[0]} : {x.Item1}" ) ) );

                foreach ( var constant in constants )
                {
                    writer.WriteLine( "\tfloat4 {0} = float4{1};", constant.Key, string.Join( ", ", constant.Value ) );
                }
                if (constants.Count > 0 )
                    writer.WriteLine();

                for (int j = 0; j < 32; j++)
                    writer.WriteLine("\tfloat4 r{0} = float4(0, 0, 0, 0);", j);

                writer.WriteLine();

                for ( ; i < lines.Length; i++ )
                {
                    string prettyLine = lines[ i ].Trim();
                    if ( prettyLine.StartsWith( "//" ) || string.IsNullOrEmpty( prettyLine ))
                        continue;

                    var args = prettyLine.Split( ',' );
                    var opArgsSplit = args[ 0 ].Split( ' ' );
                    args[ 0 ] = opArgsSplit[ 1 ];
                    var opcodeSplit = opArgsSplit[ 0 ].Split( '_' );

                    bool sat = opcodeSplit.Contains( "sat" );

                    switch ( opcodeSplit[ 0 ] )
                    {
                        case "if":
                            writer.WriteLine( "\t{{\n\tif ({0})\n{{", args[ 0 ] );
                            break;

                        case "else":
                            writer.WriteLine("\t{\n\telse\n{");
                            break;

                        case "endif":
                            writer.WriteLine("}");
                            break;

                        default:
                            writer.Write( "\t{0} = ", args[ 0 ] );
                            if ( sat )
                                writer.Write( "saturate(" );

                            for ( int j = 1; j < args.Length; j++ )
                            {
                                var swizzleSplit = args[ j ].Split( '.' );
                                var argSplit = swizzleSplit[ 0 ].Split( '_' );

                                args[ j ] = argSplit[ 0 ].Trim();
                                if ( swizzleSplit.Length > 1 )
                                    args[ j ] += "." + swizzleSplit[ 1 ].Trim();
                                if ( argSplit.Contains( "abs" ) )
                                    args[ j ] = $"abs({args[ j ]})";
                            }

                            switch ( opcodeSplit[ 0 ] )
                            {
                                case "abs":
                                    writer.Write( "abs({0})", args[ 1 ] );
                                    break;

                                case "add":
                                    writer.Write( "{0} + {1}", args[ 1 ], args[ 2 ] );
                                    break;

                                case "cmp":
                                    writer.Write( "{0} >= 0 ? {1} : {2}", args[ 1 ], args[ 2 ], args[ 3 ] );
                                    break;


                                default:
                                    Console.WriteLine( "Unimplemented instruction: {0}", opcodeSplit[ 0 ] );
                                    writer.Write( "/* Unimplemented */ {0}({1})", opcodeSplit[0], string.Join(", ", args.Skip( 1 )) );
                                    break;
                            }
                            if (sat)
                                writer.Write(")");
                            writer.WriteLine(";");
                            break;
                    }
                }

                writer.WriteLine( "}" );
            }
        }
    }
}