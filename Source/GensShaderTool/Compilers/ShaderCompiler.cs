﻿namespace GensShaderTool.Compilers;

public class ShaderCompiler
{
    private static readonly string[] sPixelShaderSubPermutations =
    {
        "NoLight",
        "NoGI",
        "ConstTexCoord"
    };

    private static readonly string[] sVertexShaderSubPermutations =
    {
        "ConstTexCoord"
    };

    private class ShaderCompilerPermutation
    {
        public string FilePath;
        public string DirectoryPath;
        public D3DIncludeHandle Handle;

        public string Name;
        public IShader Shader;
        public D3DShaderMacro[] Macros;
    }

    public static void Compile(ArchiveDatabase archiveDatabase, IReadOnlyList<(string SourceFilePath, IShader Shader)> shaders)
    {
        var existingCache = new ShaderCompilerCache();
        var populatingCache = new ShaderCompilerCache();

        existingCache.Load(archiveDatabase.Contents.FirstOrDefault(x =>
            x.Name.Equals(ShaderCompilerCache.FileName, StringComparison.OrdinalIgnoreCase))?.Data);

        using var includeFactory = new D3DIncludeFactory();
        var permutations = new List<ShaderCompilerPermutation>();

        Parallel.ForEach(shaders, tuple =>
        {
            var (filePath, shader) = tuple;

            if (existingCache.Contains(populatingCache.Add(includeFactory.Cache, filePath)) & // on purpose!
                existingCache.Contains(populatingCache.Add(shader)))
                return;

            string fullPath = Path.GetFullPath(filePath);
            string directoryPath = Path.GetDirectoryName(fullPath);
            includeFactory.Cache.Get(fullPath, out var handle);

            var stringBuilder = StringBuilderCache.Acquire();
            var pixelShader = shader as IPixelShader;

            // Create parameter data from shader if necessary
            ShaderParameterData parameterData = null;

            if (shader.Vectors.Count > 0 ||
                shader.Integers.Count > 0 ||
                shader.Booleans.Count > 0 ||
                (pixelShader != null && pixelShader.Samplers.Count > 0))
            {
                parameterData = new ShaderParameterData();

                parameterData.Vectors.AddRange(shader.Vectors);
                parameterData.Integers.AddRange(shader.Integers);
                parameterData.Booleans.AddRange(shader.Booleans);

                if (pixelShader != null)
                {
                    parameterData.Samplers.AddRange(pixelShader.Samplers.Select(x => new ShaderParameter
                    {
                        Name = x.Unit,
                        Index = (byte)(x.Index |
                                       (x.Index << 4)), // upper 4 bits correspond to the index in mrgTexcoordIndex
                    }));
                }

                archiveDatabase.Add(new DatabaseData
                {
                    Name = shader.Name + shader.ParameterExtension,
                    Data = parameterData.Save()
                }, ConflictPolicy.Replace);
            }

            // This means we are processing a post effect shader
            if (shader.Permutations.Count == 0)
            {
                var shaderData = new ShaderData
                {
                    CodeName = shader.Name
                };

                shaderData.ParameterNames.Add("global");

                if (parameterData != null)
                    shaderData.ParameterNames.Add(shader.Name);

                archiveDatabase.Add(
                    new DatabaseData
                    {
                        Data = shaderData.Save(),
                        Name = shader.Name + shader.Extension
                    }, ConflictPolicy.Replace);

                var compilerPermutation = new ShaderCompilerPermutation
                {
                    DirectoryPath = directoryPath,
                    FilePath = fullPath,
                    Handle = handle,

                    Name = shader.Name,
                    Shader = shader,
                    Macros = shader.Macros.Append(D3DShaderMacro.Terminator).ToArray()
                };

                lock (((IList)permutations).SyncRoot) 
                    permutations.Add(compilerPermutation);
            }

            // This means we are processing a material shader
            else
            {
                for (int i = 0; i < 1 << (pixelShader?.Samplers.Count ?? 0); i++)
                {
                    int samplers = 0;

                    if (pixelShader != null)
                    {
                        for (int j = 0; j < pixelShader.Samplers.Count; j++)
                        {
                            if ((i & (1 << j)) != 0)
                                samplers |= pixelShader.Samplers[j].BitValue;
                        }
                    }

                    for (int j = 0; j < 1 << shader.Features.Count; j++)
                    {
                        int features = 0;

                        for (int k = 0; k < shader.Features.Count; k++)
                        {
                            if ((j & (1 << k)) != 0)
                                features |= shader.Features[k].BitValue;
                        }

                        ShaderListData shaderListData = null;

                        foreach (var permutation in shader.Permutations)
                        {
                            if (!shader.ValidatePermutation(features, permutation))
                                continue;

                            if (pixelShader != null && !pixelShader.ValidateSamplers(samplers))
                                continue;

                            var macros = new List<D3DShaderMacro>(shader.Macros);

                            var shaderName = ShaderUtilities.GenerateShaderName(
                                stringBuilder, shader, samplers, features, permutation);

                            if (pixelShader != null)
                            {
                                var vertexShader =
                                    pixelShader.GetVertexShader(samplers, features, permutation);

                                if (vertexShader.Shader == null)
                                    continue;

                                if (pixelShader.Permutations.Count != 0)
                                {
                                    shaderListData ??= new ShaderListData();

                                    var pixelShaderPermutationData = new PixelShaderPermutationData
                                    {
                                        Name = permutation.Name,
                                        ShaderName = shaderName,
                                        SubPermutations = PixelShaderSubPermutations.All
                                    };

                                    // Generate vertex shader permutations
                                    foreach (var alsoPermutation in vertexShader.Shader.Permutations)
                                    {
                                        if ((vertexShader.Permutations & alsoPermutation.BitValue) == 0)
                                            continue;

                                        pixelShaderPermutationData.VertexShaderPermutations.Add(
                                            new VertexShaderPermutationData
                                            {
                                                Name = alsoPermutation.Name,
                                                ShaderName = ShaderUtilities.GenerateShaderName(stringBuilder,
                                                    vertexShader.Shader, 0, vertexShader.Features, alsoPermutation),
                                                SubPermutations = VertexShaderSubPermutations.All
                                            });
                                    }

                                    shaderListData.PixelShaderPermutations.Add(pixelShaderPermutationData);
                                }

                                // Add feature macros
                                foreach (var feature in vertexShader.Shader.Features)
                                {
                                    if ((vertexShader.Features & feature.BitValue) != 0)
                                        macros.Add(new D3DShaderMacro("HasFeature" + feature.BitName));
                                }
                            }
                            
                            // Add permutation macro
                            macros.Add(new D3DShaderMacro("IsPermutation" + permutation.BitName));

                            // Add feature macros
                            foreach (var feature in shader.Features)
                            {
                                if ((features & feature.BitValue) != 0)
                                    macros.Add(new D3DShaderMacro("HasFeature" + feature.BitName));
                            }

                            // Add sampler macros
                            if (pixelShader != null)
                            {
                                foreach (var sampler in pixelShader.Samplers)
                                {
                                    if ((samplers & sampler.BitValue) != 0)
                                        macros.Add(new D3DShaderMacro("HasSampler" + sampler.BitName));
                                }
                            }

                            // Apply sub-permutations
                            var subPermutationNames = pixelShader != null
                                ? sPixelShaderSubPermutations
                                : sVertexShaderSubPermutations;

                            for (int subPermutations = 0;
                                 subPermutations < 1 << subPermutationNames.Length;
                                 subPermutations++)
                            {
                                stringBuilder.Clear();
                                stringBuilder.Append(shaderName);

                                for (int k = 0; k < subPermutationNames.Length; k++)
                                {
                                    if ((subPermutations & (1 << k)) == 0)
                                        continue;

                                    stringBuilder.Append('_');
                                    stringBuilder.Append(subPermutationNames[k]);
                                }

                                var shaderData = new ShaderData
                                {
                                    CodeName = stringBuilder.ToString()
                                };

                                shaderData.ParameterNames.Add("global");

                                if (parameterData != null)
                                    shaderData.ParameterNames.Add(shader.Name);

                                stringBuilder.Append(shader.Extension);

                                archiveDatabase.Add(
                                    new DatabaseData
                                    {
                                        Data = shaderData.Save(),
                                        Name = stringBuilder.ToString()
                                    }, ConflictPolicy.Replace);

                                // Keep track of the current macro count as
                                // we are going to revert it to its original state
                                // down the line
                                int countBeforeSubPermutations = macros.Count;

                                // Add sub-permutation macros
                                for (int k = 0; k < subPermutationNames.Length; k++)
                                {
                                    if ((subPermutations & (1 << k)) != 0)
                                        macros.Add(new D3DShaderMacro(subPermutationNames[k]));
                                }

                                macros.Add(D3DShaderMacro.Terminator);

                                var compilerPermutation = new ShaderCompilerPermutation
                                {
                                    DirectoryPath = directoryPath,
                                    FilePath = fullPath,
                                    Handle = handle,

                                    Name = shaderData.CodeName,
                                    Shader = shader,
                                    Macros = macros.ToArray()
                                };

                                lock (((IList)permutations).SyncRoot)
                                    permutations.Add(compilerPermutation);

                                // Revert macro list to its original state
                                macros.RemoveRange(countBeforeSubPermutations, macros.Count - countBeforeSubPermutations);
                            }
                        }

                        if (shaderListData != null)
                        {
                            archiveDatabase.Add(new DatabaseData
                            {
                                Data = shaderListData.Save(),
                                Name = ShaderUtilities.GenerateShaderListName(stringBuilder, shader, samplers, features)
                            }, ConflictPolicy.Replace);
                        }
                    }
                }
            }

            StringBuilderCache.Release(stringBuilder);
        });

        int permutationIndex = 0;

        Parallel.ForEach(permutations, permutation =>
        {
            Console.WriteLine("({0}/{1}) {2}", Interlocked.Increment(ref permutationIndex), permutations.Count, permutation.Name);

            using var include = includeFactory.Create(permutation.DirectoryPath);

            int result = D3DCompiler.Compile(
                permutation.Handle.Data,
                permutation.Handle.Bytes,
                permutation.FilePath,
                permutation.Macros,
                include.Pointer,
                permutation.Shader.EntryPoint,
                permutation.Shader.Target,
                0, /*1 << 15*/
                0,
                out var blob,
                out var errorBlob);

            if (errorBlob != null)
            {
                string message = errorBlob.ConvertToString();
                Console.WriteLine(message);
                if (result < 0)
                    throw new Exception(message);
            }
            else if (result < 0)
                throw new Exception("Shader compilation failed");

            archiveDatabase.Add(new DatabaseData
            {
                Data = blob.ToArray(),
                Name = permutation.Name + permutation.Shader.CodeExtension
            }, ConflictPolicy.Replace);
        });

        archiveDatabase.Add(new DatabaseData { Name = ShaderCompilerCache.FileName, Data = populatingCache.Save() }, ConflictPolicy.Replace);
    }
}