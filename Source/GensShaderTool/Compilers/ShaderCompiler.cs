namespace GensShaderTool.Compilers;

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
        public string Name;
        public Shader Shader;
        public D3DShaderMacro[] Macros;
    }

    public static void Compile(string sourceFilePath, ArchiveDatabase archiveDatabase, IReadOnlyList<Shader> shaders)
    {
        var permutations = new List<ShaderCompilerPermutation>();

        Parallel.ForEach(shaders, shader =>
        {
            var stringBuilder = StringBuilderCache.Acquire();
            var pixelShader = shader as PixelShader;

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
                        Name = x.Name,
                        Index = (byte)(x.Index |
                                       (x.Index << 4)), // upper 4 bits correspond to the index in mrgTexcoordIndex
                    }));
                }

                archiveDatabase.Add(new DatabaseData
                {
                    Name = shader.Name + shader.ParameterExtension,
                    Data = parameterData.Save()
                });
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
                    });

                var compilerPermutation = new ShaderCompilerPermutation
                {
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
                // Use 1 as placeholder sampler for vertex shaders
                int samplerPermutationCount = 1 << (pixelShader?.Samplers.Count ?? 0);

                for (int samplers = 0; samplers < samplerPermutationCount; samplers++)
                {
                    for (int features = 0; features < 1 << shader.Features.Count; features++)
                    {
                        ShaderListData shaderListData = null;

                        foreach (var permutation in shader.Permutations)
                        {
                            if (!shader.ValidatePermutation(features, permutation))
                                continue;

                            if (pixelShader != null && !pixelShader.ValidateSamplers(samplers))
                                continue;

                            var shaderName = ShaderUtilities.GenerateShaderName(
                                stringBuilder, shader, samplers, features, permutation);

                            if (pixelShader != null)
                            {
                                var vertexShaderPair =
                                    pixelShader.GetVertexShader(samplers, features, permutation);

                                if (vertexShaderPair.Shader == null)
                                    continue;

                                if (pixelShader.Permutations.Count != 0)
                                {
                                    shaderListData ??= new ShaderListData();

                                    var pixelShaderPermutationData = new PixelShaderPermutationData
                                    {
                                        Name = permutation,
                                        ShaderName = shaderName,
                                        SubPermutations = PixelShaderSubPermutations.All
                                    };

                                    // Generate vertex shader permutations
                                    foreach (var alsoPermutation in vertexShaderPair.Shader.Permutations)
                                    {
                                        pixelShaderPermutationData.VertexShaderPermutations.Add(
                                            new VertexShaderPermutationData
                                            {
                                                Name = alsoPermutation,
                                                ShaderName = ShaderUtilities.GenerateShaderName(stringBuilder,
                                                    vertexShaderPair.Shader, 0, vertexShaderPair.Features, alsoPermutation),
                                                SubPermutations = VertexShaderSubPermutations.All
                                            });
                                    }

                                    shaderListData.PixelShaderPermutations.Add(pixelShaderPermutationData);
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

                                for (int i = 0; i < subPermutationNames.Length; i++)
                                {
                                    if ((subPermutations & (1 << i)) == 0)
                                        continue;

                                    stringBuilder.Append('_');
                                    stringBuilder.Append(subPermutationNames[i]);
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
                                    });

                                // Generate macros
                                var macros = new List<D3DShaderMacro>(shader.Macros);

                                // Add permutation macro
                                macros.Add(new D3DShaderMacro("is_permutation_" + permutation));

                                // Add feature macros
                                for (int i = 0; i < shader.Features.Count; i++)
                                {
                                    if ((features & (1 << i)) != 0)
                                        macros.Add(new D3DShaderMacro("has_feature_" + shader.Features[i]));
                                }

                                // Add sampler macros
                                if (pixelShader != null)
                                {
                                    for (int i = 0; i < pixelShader.Samplers.Count; i++)
                                    {
                                        if ((samplers & (1 << i)) == 0)
                                            continue;

                                        string macro = "has_sampler_" + pixelShader.Samplers[i].Name;

                                        // Sampler names might be duplicated. In that case,
                                        // we need to suffix it with an index to prevent conflicts.
                                        for (int j = 1; macros.Any(x => x.Name.Equals(macro)); j++)
                                            macro = $"has_sampler_{pixelShader.Samplers[i].Name}{j}";

                                        macros.Add(new D3DShaderMacro(macro));
                                    }
                                }

                                // Add sub-permutation macros
                                for (int i = 0; i < subPermutationNames.Length; i++)
                                {
                                    if ((subPermutations & (1 << i)) != 0)
                                        macros.Add(new D3DShaderMacro(subPermutationNames[i]));
                                }

                                macros.Add(D3DShaderMacro.Terminator);

                                var compilerPermutation = new ShaderCompilerPermutation
                                {
                                    Name = shaderData.CodeName,
                                    Shader = shader,
                                    Macros = macros.ToArray()
                                };

                                lock (((IList)permutations).SyncRoot)
                                    permutations.Add(compilerPermutation);
                            }
                        }

                        if (shaderListData != null)
                        {
                            archiveDatabase.Add(new DatabaseData
                            {
                                Data = shaderListData.Save(),
                                Name = ShaderUtilities.GenerateShaderListName(stringBuilder, shader, samplers, features)
                            });
                        }
                    }
                }
            }

            StringBuilderCache.Release(stringBuilder);
        });

        var sourceData = File.ReadAllText(sourceFilePath);

        int permutationIndex = 0;

        Parallel.ForEach(permutations, permutation =>
        {
            Console.WriteLine("({0}/{1}) {2}", Interlocked.Increment(ref permutationIndex), permutations.Count, permutation.Name);

            int result = D3DCompiler.Compile(
                sourceData,
                sourceData.Length,
                sourceFilePath,
                permutation.Macros,
                new IntPtr(1), // D3D_COMPILE_STANDARD_FILE_INCLUDE 
                permutation.Shader.EntryPoint,
                permutation.Shader.Target,
                0,
                0,
                out var blob,
                out var errorBlob);

            if (result < 0)
                throw new Exception(errorBlob?.ConvertToString() ?? string.Empty);

            archiveDatabase.Add(new DatabaseData
            {
                Data = blob.ToArray(),
                Name = permutation.Name + permutation.Shader.CodeExtension
            });
        });
    }
}