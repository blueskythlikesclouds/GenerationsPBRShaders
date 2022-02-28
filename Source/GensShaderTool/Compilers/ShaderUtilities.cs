namespace GensShaderTool.Compilers;

public class ShaderUtilities
{
    private static bool AppendSamplers(StringBuilder stringBuilder, PixelShader pixelShader, int samplers)
    {
        if (pixelShader == null || samplers == 0 || pixelShader.Samplers.Count == 0)
            return false;

        int position = stringBuilder.Length;

        for (int i = 0; i < pixelShader.Samplers.Count; i++)
        {
            if ((samplers & (1 << i)) != 0)
                stringBuilder.Append(pixelShader.Samplers[i].Token);
        }

        // If any of the tokens are valid, insert an underscore
        // Example case for lack of underscore is ChrEyeCDRF
        if (stringBuilder.Length == position)
            return false;

        stringBuilder.Insert(position, '_');
        return true;
    }

    public static bool GeneratesOriginalName(Shader shader)
    {
        return shader.Features.Count == 0 && (shader is not PixelShader pixelShader || pixelShader.Samplers.Count == 0);
    }

    public static string GenerateShaderName(StringBuilder stringBuilder, Shader shader, int samplers, int features, string permutation)
    {
        if (GeneratesOriginalName(shader))
            return shader.Name;

        stringBuilder.Clear();
        stringBuilder.Append(shader.Name);

        bool underscore = AppendSamplers(stringBuilder, shader as PixelShader, samplers);

        if (shader.Features.Count == 0) 
            return stringBuilder.ToString();

        if (!underscore)
            stringBuilder.Append('_');

        stringBuilder.Append('@');
        for (int i = 0; i < shader.Features.Count; i++)
        {
            if ((features & (1 << i)) != 0)
                stringBuilder.Append(shader.Features[i]);
        }

        stringBuilder.Append('@');

        if (!string.IsNullOrEmpty(permutation) && permutation != "none" && permutation != "default")
            stringBuilder.Append(permutation[0]);

        return stringBuilder.ToString();
    }

    public static string GenerateShaderListName(StringBuilder stringBuilder, Shader shader, int samplers, int features)
    {
        if (GeneratesOriginalName(shader))
            return shader.Name;

        stringBuilder.Clear();
        stringBuilder.Append(shader.Name);

        AppendSamplers(stringBuilder, shader as PixelShader, samplers);

        if (features != 0)
        {
            stringBuilder.Append('[');
            for (int i = 0; i < shader.Features.Count; i++)
            {
                if ((features & (1 << i)) != 0)
                    stringBuilder.Append(shader.Features[i]);
            }

            stringBuilder.Append(']');
        }

        stringBuilder.AppendFormat(".shader-list");

        return stringBuilder.ToString();
    }
}