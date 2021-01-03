using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using GensShaderTool.Extensions;

namespace GensShaderTool
{
    public static class ShaderCompiler
    {
        private static readonly string[] sPixelShaderMiragePermutations =
        {
            "NoLight",
            "NoGI",
            "ConstTexCoord"
        };

        private static readonly string[] sVertexShaderMiragePermutations =
        {
            "ConstTexCoord"
        };

        public static void Compile( string hlslFilePath, string outputDirectory,
            IReadOnlyList<IShaderInfo> shaders, ShaderParameterSet globalParameterSet, bool isXbox360 = false )
        {
            int maxConstantIndex = globalParameterSet.SingleParameters.Max( x => x.Index ) + 1;

            Directory.CreateDirectory( outputDirectory );

            string asmDirectoryPath = Path.Combine( outputDirectory, "asm" );
            Directory.CreateDirectory( asmDirectoryPath );

            var shaderLists = new List<ShaderList>();
            var permutations = new List<ShaderConverterPermutation>();
            var mainParameterSets = new Dictionary<IShaderInfo, ShaderParameterSet>();

            var stringBuilder = new StringBuilder();

            foreach ( var shader in shaders )
            {
                for ( int iterationIndex = 0; iterationIndex < Math.Max( 0, shader.IterationCount ); iterationIndex++ )
                { 
                    if ( shader.Samplers.Count > 0 && shader.Type == ShaderType.Vertex )
                        throw new InvalidDataException( "A vertex shader cannot have samplers" );

                    if ( shader.Samplers.Count > 16 )
                        throw new IndexOutOfRangeException( "A pixel shader can have maximum 16 samplers" );

                    for ( int i = 0; i < 1 << shader.Samplers.Count; i++ )
                    {
                        if ( shader.Techniques.Count > 0 && shader.Type == ShaderType.Pixel )
                        {
                            stringBuilder.Clear();
                            stringBuilder.Append( shader.Name );

                            if ( shader.IterationCount > 1 )
                                stringBuilder.AppendFormat( "_{0}", iterationIndex );

                            if ( i != 0 )
                            {
                                stringBuilder.Append( '_' );

                                for ( int j = 0; j < shader.Samplers.Count; j++ )
                                    if ( ( ( i >> j ) & 1 ) != 0 )
                                        stringBuilder.Append( shader.Samplers[ j ].Suffix );
                            }

                            string name = stringBuilder.ToString().Trim( '_' );

                            var shaderList = new ShaderList();
                            shaderList.Name = name;

                            bool any = false;

                            foreach ( var technique in shader.Techniques )
                            {
                                if ( !shader.ValidatePermutation( ( ushort ) i, technique ) )
                                    continue;

                                any = true;
                                for ( int j = 0; j < 1 << sPixelShaderMiragePermutations.Length; j++ )
                                {
                                    permutations.Add( new ShaderConverterPermutation
                                    {
                                        Name = name,
                                        ShaderList = shaderList,
                                        Shader = shader,
                                        Technique = technique,
                                        SamplerBits = i,
                                        MiragePermutationBits = j,
                                        IterationIndex = iterationIndex
                                    } );
                                }
                            }

                            if ( any )
                                shaderLists.Add( shaderList );
                        }
                        else
                        {
                            if ( shader.Techniques.Count > 0 && shader.Type == ShaderType.Vertex )
                                throw new InvalidDataException( "A vertex shader cannot have pixel shader techniques" );

                            if ( shader.Type == ShaderType.Pixel && !shader.ValidatePermutation( ( ushort ) i, null ) )
                                continue;

                            var miragePermutations = shader.Type == ShaderType.Vertex
                                ? sVertexShaderMiragePermutations
                                : Array.Empty<string>();

                            for ( int j = 0; j < 1 << miragePermutations.Length; j++ )
                                permutations.Add( new ShaderConverterPermutation
                                {
                                    Name = $"{shader.Name}{( shader.IterationCount > 1 ? $"_{iterationIndex}" : "" )}",
                                    Shader = shader,
                                    MiragePermutationBits = j,
                                    IterationIndex = iterationIndex
                                } );
                        }
                    }
                }

                mainParameterSets.Add( shader, new ShaderParameterSet() );
            }

            var samplerMaps =
                shaders.ToImmutableDictionary( x => x,
                    x => x.Samplers.ToImmutableDictionary( y => y.Name, y => y, StringComparer.OrdinalIgnoreCase ) );

            var constantMaps =
                shaders.ToImmutableDictionary( x => x,
                    x => x.Constants.ToImmutableHashSet( StringComparer.OrdinalIgnoreCase ) );

            var samplerDefMap = shaders.SelectMany( x => x.Samplers ).ToDictionary( x => x, x =>
            {
                stringBuilder.Clear();
                stringBuilder.Append( "Has" );
                stringBuilder.Append( char.ToUpperInvariant( x.Name[ 0 ] ) );

                stringBuilder.Append( x.Name.EndsWith( "Sampler", StringComparison.OrdinalIgnoreCase )
                    ? x.Name.AsSpan().Slice( 1, x.Name.Length - 8 )
                    : x.Name.AsSpan().Slice( 1 ) );

                return stringBuilder.ToString();
            } );

            var allSamplerNames = shaders.SelectMany( x => x.Samplers ).Select( x => x.Name )
                .ToImmutableHashSet( StringComparer.OrdinalIgnoreCase );

            var allConstantNames = shaders.SelectMany( x => x.Constants )
                .ToImmutableHashSet( StringComparer.OrdinalIgnoreCase );

            var allDefs = shaders.SelectMany( x => x.Definitions.Append( $"Is{x.Name}" ) )
                .Concat( samplerDefMap.Values )
                .Concat( sPixelShaderMiragePermutations ).Append( "IsXbox360" )
                .ToImmutableHashSet( StringComparer.OrdinalIgnoreCase );

            var globalParameterNames = globalParameterSet.BoolParameters.Concat( globalParameterSet.IntParameters )
                .Concat( globalParameterSet.SamplerParameters ).Concat( globalParameterSet.SingleParameters )
                .Select( x => x.Name ).ToImmutableHashSet( StringComparer.OrdinalIgnoreCase );

            Parallel.ForEach( permutations, permutation =>
            {
                char typeChar = permutation.Shader.Type == ShaderType.Vertex ? 'v' : 'p';

                var nameBuilder = new StringBuilder( permutation.Name.Length * 4 );
                nameBuilder.Append( permutation.Name );
                if ( permutation.Technique != null && permutation.Shader.Techniques.Count > 1 )
                    nameBuilder.AppendFormat( "@@{0}", permutation.Technique.Suffix );

                var defs = new HashSet<string>( allDefs.Count / 2, StringComparer.OrdinalIgnoreCase );

                defs.Add( $"Is{permutation.Shader.Name}" );
                foreach ( string def in permutation.Shader.Definitions )
                    defs.Add( def );

                if ( isXbox360 )
                    defs.Add( "IsXbox360" );

                for ( int i = 0; i < permutation.Shader.Samplers.Count; i++ )
                    if ( ( ( permutation.SamplerBits >> i ) & 1 ) != 0 )
                        defs.Add( samplerDefMap[ permutation.Shader.Samplers[ i ] ] );

                var miragePermutations = permutation.Shader.Type == ShaderType.Vertex
                    ? sVertexShaderMiragePermutations
                    : sPixelShaderMiragePermutations;

                for ( int i = 0; i < miragePermutations.Length; i++ )
                {
                    if ( ( ( permutation.MiragePermutationBits >> i ) & 1 ) == 0 )
                        continue;

                    string permutationName = miragePermutations[ i ];
                    defs.Add( permutationName );
                    nameBuilder.AppendFormat( "_{0}", permutationName );
                }

                var defsBuilder = new StringBuilder( allDefs.Count * 8 );
                foreach ( string def in allDefs )
                    defsBuilder.AppendFormat( "/D {0}={1} ", def, defs.Contains( def ) ? 1 : 0 );

                for ( int i = 0; i < permutation.Shader.Samplers.Count; i++ )
                    defsBuilder.AppendFormat( "/D {0}=s{1} /D {0}Index={1} ",
                        permutation.Shader.Samplers[ i ].Name.Replace( "Sampler", "Register" ), i );

                for ( int i = 0; i < permutation.Shader.Constants.Count; i++ )
                    defsBuilder.AppendFormat( "/D {0}Register=c{1} ", permutation.Shader.Constants[ i ],
                        maxConstantIndex + i );

                var samplerMap = samplerMaps[ permutation.Shader ];
                foreach ( string samplerName in allSamplerNames.Where( x => !samplerMap.ContainsKey( x ) ) )
                    defsBuilder.AppendFormat( "/D {0}=s15 /D {0}Index=15 ",
                        samplerName.Replace( "Sampler", "Register" ) );

                var constantMap = constantMaps[ permutation.Shader ];
                foreach ( string constantName in allConstantNames.Where( x => !constantMap.Contains( x ) ) )
                    defsBuilder.AppendFormat( "/D {0}Register=c255 ", constantName );

                defsBuilder.AppendFormat( "/D IterationIndex={0}", permutation.IterationIndex );

                string defines = defsBuilder.ToString();
                string name = nameBuilder.ToString();

                string codeFilePath =
                    Path.Combine( outputDirectory, name + $".{( isXbox360 ? 'x' : 'w' )}{typeChar}u" );
                string asmFilePath = codeFilePath + ".asm";

                var processStartInfo = new ProcessStartInfo
                {
                    FileName = isXbox360 ? "fxc-xbox360" : "fxc",
                    Arguments =
                        $"{( isXbox360 ? "/Xmaxtempreg:64 /Gfa" : "" )} /T {typeChar}s_3_0 /Zpr {defines} \"{hlslFilePath}\" /Fo \"{codeFilePath}\" /Fc \"{asmFilePath}\"",
                    RedirectStandardError = true
                };

                using ( var process = Process.Start( processStartInfo ) )
                {
                    process.WaitForExit();

                    if ( !File.Exists( asmFilePath ) )
                        throw new FileNotFoundException( process.StandardError.ReadToEnd(), asmFilePath );
                }

                bool anyLocal = false;
                bool anyGlobal = false;

                void MoveParameters( List<ShaderParameter> source, List<ShaderParameter> destination,
                    Action<ShaderParameter> evaluation = null )
                {
                    foreach ( var parameter in source )
                    {
                        if ( globalParameterNames.Contains( parameter.Name ) )
                        {
                            anyGlobal = true;
                            continue;
                        }

                        anyLocal = true;
                        evaluation?.Invoke( parameter );
                        destination.Add( parameter );
                    }
                }

                var parameterSet = ShaderParameterConverter.ParseAssemblyComments( File.ReadAllLines( asmFilePath ) );

                var mainParameterSet = mainParameterSets[ permutation.Shader ];

                lock ( mainParameterSet )
                {
                    MoveParameters( parameterSet.BoolParameters, mainParameterSet.BoolParameters );

                    MoveParameters( parameterSet.IntParameters, mainParameterSet.IntParameters );

                    MoveParameters( parameterSet.SingleParameters, mainParameterSet.SingleParameters );

                    MoveParameters( parameterSet.SamplerParameters, mainParameterSet.SamplerParameters, parameter =>
                    {
                        if ( !samplerMap.TryGetValue( parameter.Name, out var info ) )
                            return;

                        parameter.Name = info.Unit;
                    } );
                }

                var shader = new Shader();
                shader.FileName = name;

                if ( anyGlobal )
                    shader.ParameterFileNames.Add( "global" );

                if ( anyLocal )
                    shader.ParameterFileNames.Add( permutation.Shader.Name );

                shader.Save( Path.Combine( outputDirectory,
                    name + $".{( permutation.Shader.Type == ShaderType.Vertex ? "vertex" : "pixel" )}shader" ) );

                File.Move( asmFilePath,
                    Path.Combine( asmDirectoryPath, name + $".{( isXbox360 ? 'x' : 'w' )}{typeChar}u.asm" ), true );

                if ( permutation.Shader.Type != ShaderType.Pixel || permutation.MiragePermutationBits != 0 ||
                     permutation.ShaderList == null || permutation.Technique == null )
                    return;

                var pixelShaderPermutation = new PixelShaderPermutation
                {
                    Flags = 0xFF,
                    ShaderName = name,
                    Technique = permutation.Technique.Name
                };

                pixelShaderPermutation.VertexShaderPermutations.AddRange(
                    permutation.Technique.VertexShaderPermutations );

                lock ( permutation.ShaderList )
                {
                    permutation.ShaderList.PixelShaderPermutations.Add( pixelShaderPermutation );
                }
            } );

            foreach ( var (info, mainParameterSet) in mainParameterSets )
            {
                var distinctParameterSet = new ShaderParameterSet();

                distinctParameterSet.BoolParameters.AddRange( mainParameterSet.BoolParameters.GroupBy( x => x.Index )
                    .Select( x => x.First() ).OrderBy( x => x.Index ) );

                distinctParameterSet.IntParameters.AddRange( mainParameterSet.IntParameters.GroupBy( x => x.Index )
                    .Select( x => x.First() ).OrderBy( x => x.Index ) );

                distinctParameterSet.SingleParameters.AddRange( mainParameterSet.SingleParameters
                    .GroupBy( x => x.Index )
                    .Select( x => x.First() ).OrderBy( x => x.Index ) );

                distinctParameterSet.SamplerParameters.AddRange( mainParameterSet.SamplerParameters
                    .GroupBy( x => x.Index )
                    .Select( x => x.First() ).OrderBy( x => x.Index ) );

                if ( distinctParameterSet.IsEmpty() )
                    continue;

                distinctParameterSet.Save( Path.Combine( outputDirectory,
                    info.Name + $".{( info.Type == ShaderType.Vertex ? 'v' : 'p' )}sparam" ) );
            }

            foreach ( var shaderList in shaderLists )
                shaderList.Save( Path.Combine( outputDirectory, shaderList.Name + ".shader-list" ) );
        }

        private class ShaderConverterPermutation
        {
            public int MiragePermutationBits;
            public string Name;
            public int SamplerBits;
            public int IterationIndex;
            public IShaderInfo Shader;
            public ShaderList ShaderList;
            public PixelShaderTechniqueInfo Technique;
        }
    }
}