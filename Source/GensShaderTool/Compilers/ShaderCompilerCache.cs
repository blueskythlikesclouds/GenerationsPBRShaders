using K4os.Hash.xxHash;

namespace GensShaderTool.Compilers;

public class ShaderCompilerCache
{
    private static readonly byte[] sIncludeToken = { 0x23, 0x69, 0x6E, 0x63, 0x6C, 0x75, 0x64, 0x65 }; // #include
    private const byte cQuotationToken = (byte)'"';

    public const string FileName = "shader_compiler_cache.bin";

    private readonly ConcurrentDictionary<string, IdHashPair> mIdHashPairs = new(StringComparer.OrdinalIgnoreCase);

    public void Load(byte[] bytes)
    {
        if (bytes == null || bytes.Length == 0)
            return;

        Mask(bytes);

        using var stream = new MemoryStream(bytes);
        using var reader = new BinaryReader(stream, Encoding.UTF8);

        int count = reader.ReadInt32();
        for (int i = 0; i < count; i++)
        {
            string id = reader.ReadString();
            ulong hash = reader.ReadUInt64();

            mIdHashPairs[id] = new IdHashPair { Id = id, Hash = hash };
        }
    }

    public byte[] Save()
    {
        using var stream = new MemoryStream();
        using var writer = new BinaryWriter(stream, Encoding.UTF8);

        writer.Write(mIdHashPairs.Count);
        foreach (var pair in mIdHashPairs.Values)
        {
            writer.Write(pair.Id);
            writer.Write(pair.Hash);
        }

        return Mask(stream.ToArray());
    }

    private static byte[] Mask(byte[] bytes)
    {
        for (int i = 0; i < bytes.Length; i++)
            bytes[i] ^= 0b10101010;

        return bytes;
    }

    public bool Contains(IdHashPair pair)
    {
        return mIdHashPairs.TryGetValue(pair.Id, out var alsoPair) && pair.Hash == alsoPair.Hash;
    }

    public IdHashPair Add(string id, Func<ulong> hasher)
    {
        if (mIdHashPairs.TryGetValue(id, out var pair))
            return pair;

        pair.Id = id;
        pair.Hash = hasher();

        mIdHashPairs.TryAdd(id, pair);

        return pair;
    }

    public IdHashPair Add(IShader shader)
    {
        return Add(GetId(shader), () =>
        {
            var hash = new XXH64();

            hash.Update(shader.Name);
            hash.Update(shader.EntryPoint);
            hash.Update(shader.Target);
            hash.Update(shader.Extension);
            hash.Update(shader.CodeExtension);
            hash.Update(shader.ParameterExtension);

            foreach (var param in shader.Vectors)
            {
                hash.Update(param.Name);
                hash.Update(param.Index);
                hash.Update(param.Size);
            }        
            
            foreach (var param in shader.Integers)
            {
                hash.Update(param.Name);
                hash.Update(param.Index);
                hash.Update(param.Size);
            }       
            
            foreach (var param in shader.Booleans)
            {
                hash.Update(param.Name);
                hash.Update(param.Index);
                hash.Update(param.Size);
            }

            foreach (var feature in shader.Features)
            {
                hash.Update(feature.BitValue);
                hash.Update(feature.BitName);
                hash.Update(feature.Suffix);
            }       
            
            foreach (var permutation in shader.Permutations)
            {
                hash.Update(permutation.BitValue);
                hash.Update(permutation.BitName);
                hash.Update(permutation.Name);
                hash.Update(permutation.Suffix);
            }

            foreach (var macro in shader.Macros)
            {
                hash.Update(macro.Name);
                hash.Update(macro.Definition);
            }

            return hash.Digest();
        });
    }

    public unsafe IdHashPair Add(D3DIncludeCache includeCache, string filePath)
    {
        string fullPath = Path.GetFullPath(filePath);
        includeCache.Get(fullPath, out var handle);

        return Add(fullPath, () =>
        {
            string directoryPath = Path.GetDirectoryName(fullPath);

            var span = new Span<byte>((byte*)handle.Data, handle.Bytes);
            var xxHash = new XXH64();

            xxHash.Update(span);

            int index;
            while ((index = span.IndexOf(sIncludeToken)) != -1)
            {
                span = span[(index + 1)..];
                index = span.IndexOf(cQuotationToken);
                if (index == -1)
                    throw new InvalidDataException("Unable to locate preceding quotation mark in include directive");

                span = span[(index + 1)..];
                index = span.IndexOf(cQuotationToken);
                if (index == -1)
                    throw new InvalidDataException("Unable to locate ending quotation mark in include directive");

                string includeFileName = Encoding.UTF8.GetString(span[..index]);
                string includeFilePath = Path.Combine(directoryPath, includeFileName);
                var includePair = Add(includeCache, includeFilePath);

                xxHash.Update(includePair.Hash);
            }

            return xxHash.Digest();
        });
    }

    public static string GetId(IShader shader)
    {
        var type = shader.GetType();
        return type.FullName ?? type.Name;
    }

    public struct IdHashPair
    {
        public string Id;
        public ulong Hash;
    }
}