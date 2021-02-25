using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using GensShaderTool.Extensions;
using SharpGen.Runtime;
using Vortice.D3DCompiler;
using Vortice.Direct3D;

using ShaderCompileIncludeCache = System.Collections.Generic.Dictionary<string, byte[]>;

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

        public static void Compile( string hlslFilePath, ArchiveDatabase archiveDatabase,
            IReadOnlyList<IShaderInfo> shaders, ShaderParameterSet globalParameterSet, ShaderFlags flags = ShaderFlags.None )
        {
            var cache = new ShaderCompileIncludeCache();

            string hlslFileName = Path.GetFileName(hlslFilePath);
            string hlslDirectoryPath = Path.GetDirectoryName(hlslFilePath);
            string hlslFileData = File.ReadAllText(hlslFilePath);

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

            var allDefines = shaders.SelectMany( x => x.Definitions.Append( $"Is{x.Name}" ) )
                .Concat( samplerDefMap.Values )
                .Concat( sPixelShaderMiragePermutations )
                .ToImmutableHashSet( StringComparer.OrdinalIgnoreCase );

            var globalParameterNames = globalParameterSet.BoolParameters.Concat( globalParameterSet.IntParameters )
                .Concat( globalParameterSet.SamplerParameters ).Concat( globalParameterSet.SingleParameters )
                .Select( x => x.Name ).ToImmutableHashSet( StringComparer.OrdinalIgnoreCase );

            int currentPermutationIndex = 0;

            Parallel.ForEach(permutations, permutation =>
            {
                char typeChar = permutation.Shader.Type == ShaderType.Vertex ? 'v' : 'p';

                var nameBuilder = new StringBuilder(permutation.Name.Length * 4);
                nameBuilder.Append(permutation.Name);

                if (permutation.Technique != null && permutation.Shader.Techniques.Count > 1)
                    nameBuilder.AppendFormat("@@{0}", permutation.Technique.Suffix);

                var permutationDefines = new HashSet<string>(allDefines.Count / 2, StringComparer.OrdinalIgnoreCase);

                permutationDefines.Add($"Is{permutation.Shader.Name}");
                foreach (string def in permutation.Shader.Definitions)
                    permutationDefines.Add(def);

                for (int i = 0; i < permutation.Shader.Samplers.Count; i++)
                {
                    if (((permutation.SamplerBits >> i) & 1) != 0)
                        permutationDefines.Add(samplerDefMap[permutation.Shader.Samplers[i]]);
                }

                var miragePermutations = permutation.Shader.Type == ShaderType.Vertex
                    ? sVertexShaderMiragePermutations
                    : sPixelShaderMiragePermutations;

                for (int i = 0; i < miragePermutations.Length; i++)
                {
                    if (((permutation.MiragePermutationBits >> i) & 1) == 0)
                        continue;

                    string permutationName = miragePermutations[i];
                    permutationDefines.Add(permutationName);
                    nameBuilder.AppendFormat("_{0}", permutationName);
                }

                var defines = new List<ShaderMacro>(allDefines.Count);

                foreach (string define in allDefines)
                    defines.Add(new ShaderMacro(define, permutationDefines.Contains(define) ? "1" : "0"));

                defines.Add(new ShaderMacro("IterationIndex", permutation.IterationIndex));

                string name = nameBuilder.ToString();

                var result = Compiler.Compile(hlslFileData, defines.ToArray(), new ShaderCompilerInclude(cache, hlslDirectoryPath), "main", hlslFilePath,
                    $"{typeChar}s_3_0", ShaderFlags.PackMatrixRowMajor | flags, out var blob, out var errorBlob);

                if (result.Failure)
                    throw new Exception(errorBlob.ConvertToString());

                archiveDatabase.LockedAdd(
                    new DatabaseData { Name = $"{name}.w{typeChar}u", Data = blob.GetBytes() });

                bool anyLocal = false;
                bool anyGlobal = false;

                void MoveParameters(List<ShaderParameter> source, List<ShaderParameter> destination,
                    Action<ShaderParameter> evaluation = null)
                {
                    foreach (var parameter in source)
                    {
                        if (globalParameterNames.Contains(parameter.Name))
                        {
                            anyGlobal = true;
                            continue;
                        }

                        anyLocal = true;
                        evaluation?.Invoke(parameter);
                        destination.Add(parameter);
                    }
                }

                var samplerMap = samplerMaps[permutation.Shader];

                var parameterSet = ShaderParameterConverter.ReadConstantTable(new MemoryStream(blob.GetBytes()));

                var mainParameterSet = mainParameterSets[permutation.Shader];

                lock (mainParameterSet)
                {
                    MoveParameters(parameterSet.BoolParameters, mainParameterSet.BoolParameters);

                    MoveParameters(parameterSet.IntParameters, mainParameterSet.IntParameters);

                    MoveParameters(parameterSet.SingleParameters, mainParameterSet.SingleParameters);

                    MoveParameters(parameterSet.SamplerParameters, mainParameterSet.SamplerParameters, parameter =>
                    {
                        if (!samplerMap.TryGetValue(parameter.Name, out var info))
                            return;

                        parameter.Name = info.Unit;
                    });
                }

                var shader = new Shader();
                shader.FileName = name;

                if (anyGlobal)
                    shader.ParameterFileNames.Add("global");

                if (anyLocal)
                    shader.ParameterFileNames.Add(permutation.Shader.Name);

                archiveDatabase.LockedAdd(
                    new DatabaseData
                    {
                        Name = $"{name}.{(permutation.Shader.Type == ShaderType.Vertex ? "vertex" : "pixel")}shader",
                        Data = shader.Save()
                    });

                Console.WriteLine("({0}/{1}): {2}", Interlocked.Increment(ref currentPermutationIndex), permutations.Count, name);

                if (permutation.Shader.Type != ShaderType.Pixel || permutation.MiragePermutationBits != 0 ||
                    permutation.ShaderList == null || permutation.Technique == null)
                    return;

                var pixelShaderPermutation = new PixelShaderPermutation
                {
                    Flags = 0xFF,
                    ShaderName = name,
                    Technique = permutation.Technique.Name
                };

                pixelShaderPermutation.VertexShaderPermutations.AddRange(
                    permutation.Technique.VertexShaderPermutations);

                lock (permutation.ShaderList)
                    permutation.ShaderList.PixelShaderPermutations.Add(pixelShaderPermutation);
            });

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

                archiveDatabase.Contents.Add(new DatabaseData
                {
                    Name = $"{info.Name}.{( info.Type == ShaderType.Vertex ? 'v' : 'p' )}sparam",
                    Data = distinctParameterSet.Save()
                });
            }

            foreach (var shaderList in shaderLists)
                archiveDatabase.Contents.Add(new DatabaseData
                {
                    Name = shaderList.Name + ".shader-list",
                    Data = shaderList.Save()
                });
        }

        public static byte[] Compile(string hlslData, ShaderType type, ShaderFlags flags = ShaderFlags.None)
        {
            var result = Compiler.Compile(hlslData, null, null, "main", null, type == ShaderType.Vertex ? "vs_3_0" : "ps_3_0", flags,
                out var blob, out var errorBlob);

            if (result.Failure)
                throw new Exception(errorBlob.ConvertToString());

            return blob.GetBytes();
        }

        public static byte[] CompileFromFile(string sourceFilePath, ShaderType type, ShaderFlags flags = ShaderFlags.None)
        {
            return Compile(File.ReadAllText(sourceFilePath), type, flags);
        }

        public static void Compile(string hlslData, ShaderType type, string destinationFilePath, ShaderFlags flags = ShaderFlags.None)
        {
            var result = Compiler.Compile(hlslData, null, null, "main", null, type == ShaderType.Vertex ? "ps_3_0" : "vs_3_0", flags,
                out var blob, out var errorBlob);

            if (result.Failure)
                throw new Exception(errorBlob.ConvertToString());

            Compiler.WriteBlobToFile(blob, destinationFilePath, true);
        }

        public static void CompileFromFile(string sourceFilePath, ShaderType type, string destinationFilePath, ShaderFlags flags = ShaderFlags.None)
        {
            Compile(File.ReadAllText(sourceFilePath), type, destinationFilePath, flags);
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

    public class ShaderCompilerInclude : Include
    {
        private readonly ShaderCompileIncludeCache mCache;
        private readonly Stack<string> mDirectoryPaths;

        public ShadowContainer Shadow { get; set; }

        public Stream Open(IncludeType type, string fileName, Stream parentStream)
        {
            string fullPath = Path.GetFullPath(Path.Combine(mDirectoryPaths.Peek(), fileName));
            mDirectoryPaths.Push(Path.GetDirectoryName(fullPath));

            byte[] data;

            lock (mCache)
            {
                if (!mCache.TryGetValue(fullPath, out data))
                    mCache[fullPath] = data = File.ReadAllBytes(fullPath);
            }

            return new MemoryStream(data);
        }

        public void Close(Stream stream)
        {
            mDirectoryPaths.Pop();
            stream.Close();
        }

        public void Dispose()
        {
            Shadow.Dispose();
        }

        public ShaderCompilerInclude(ShaderCompileIncludeCache cache, string directoryPath)
        {
            mCache = cache;
            mDirectoryPaths = new Stack<string>();
            mDirectoryPaths.Push(directoryPath);
        }
    }
}