namespace GensShaderTool.Compilers;

public class ShaderUtilities
{
    private static bool AppendSamplers(StringBuilder stringBuilder, IPixelShader pixelShader, int samplers)
    {
        if (pixelShader == null || samplers == 0 || pixelShader.Samplers.Count == 0)
            return false;

        int position = stringBuilder.Length;

        foreach (var sampler in pixelShader.Samplers)
        {
            if ((samplers & sampler.BitValue) != 0)
                stringBuilder.Append(sampler.Suffix);
        }

        // If any of the suffixes are valid, insert an underscore
        // Example case for lack of underscore is ChrEyeCDRF
        if (stringBuilder.Length == position)
            return false;

        stringBuilder.Insert(position, '_');
        return true;
    }

    public static bool GeneratesOriginalName(IShader shader)
    {
        return shader.Features.Count == 0 && shader.Permutations.Count == 0 && (shader is not IPixelShader pixelShader || pixelShader.Samplers.Count == 0);
    }

    public static string GenerateShaderName(StringBuilder stringBuilder, IShader shader, int samplers, int features, IPermutation permutation)
    {
        if (GeneratesOriginalName(shader))
            return shader.Name;

        stringBuilder.Clear();
        stringBuilder.Append(shader.Name);

        if (!AppendSamplers(stringBuilder, shader as IPixelShader, samplers))
            stringBuilder.Append('_');

        stringBuilder.Append('@');
        foreach (var feature in shader.Features)
        {
            if ((features & feature.BitValue) != 0)
                stringBuilder.Append(feature.Suffix);
        }

        stringBuilder.Append('@');
        stringBuilder.Append(permutation.Suffix);

        return stringBuilder.ToString();
    }

    public static string GenerateShaderListName(StringBuilder stringBuilder, IShader shader, int samplers, int features)
    {
        if (GeneratesOriginalName(shader))
            return shader.Name;

        stringBuilder.Clear();
        stringBuilder.Append(shader.Name);

        AppendSamplers(stringBuilder, shader as IPixelShader, samplers);

        if (features != 0)
        {
            stringBuilder.Append('[');
            foreach (var feature in shader.Features)
            {
                if ((features & feature.BitValue) != 0)
                    stringBuilder.Append(feature.Suffix);
            }

            stringBuilder.Append(']');
        }

        stringBuilder.AppendFormat(".shader-list");

        return stringBuilder.ToString();
    }
}