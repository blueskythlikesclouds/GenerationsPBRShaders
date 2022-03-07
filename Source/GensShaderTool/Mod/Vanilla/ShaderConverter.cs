using System.Diagnostics;
using ShaderTranslator;

namespace GensShaderTool.Mod.Vanilla;

public class ShaderConverter : IDisposable
{
    private readonly D3DIncludeFactory mIncludeFactory = new();
    private readonly string mShaderSourceCodeDirectory;

    public ShaderConverter(string shaderSourceCodeDirectory)
    {
        mShaderSourceCodeDirectory = shaderSourceCodeDirectory;
    }

    public byte[] ConvertPostEffectShader(string name, byte[] bytes)
    {
        string translated = Translator.Translate(bytes, out bool isPixelShader);

        // Prevent average luminance getting clamped to 0-1 range.
        if (name.Contains("MeasureLuminance"))
            translated = translated.Replace("saturate", string.Empty);

        return Translator.Compile(translated, isPixelShader);
    }

    public unsafe byte[] ConvertMaterialShader(string name, byte[] bytes)
    {
        string translated = Translator.Translate(bytes, out bool isPixelShader);

        // Vertex shaders don't need any patching.
        if (!isPixelShader)
            return Translator.Compile(translated, false);

        // Split the code.
        int index = translated.IndexOf("void main(", StringComparison.Ordinal);

        string preCode = translated[..index];
        string mainCode = translated[index..];

        int codeIndex = mainCode.IndexOf('{');
        string mainCodeArgs = mainCode[..(codeIndex + 1)];
        string code = mainCode[(codeIndex + 1)..];
        string codeOriginal = code;

        // Include PBR functionality.
        preCode = "#define GLOBALS_PS_ONLY_PBR_CONSTANTS\n" +
                  "#include \"GlobalsPS.hlsli\"\n" +
                  "#include \"SharedPS.hlsli\"\n" +
                  "" +
                  "float4 GetToneMapLuminanceVec4()" +
                  "{" +
                  "    return float4(GetToneMapLuminance().xxx, 1.0);" +
                  "}" +
                  "" +
                  "float GetShadow(Texture2D<float> texShadow, SamplerState sampShadow, float2 texCoord, float depth)" +
                  "{" +
                  "    return ComputeShadow(texShadow, sampShadow, g_ShadowMapSize, g_ESMFactor, float4((texCoord - 0.5) * float2(2.0, -2.0), depth, 1.0));" +
                  "}" +
                  "\n" +
                  preCode;

        // Replace shadow map samplers/textures.
        preCode = preCode.Replace("SamplerComparisonState", "SamplerState");
        preCode = preCode.Replace("4> g_VerticalShadowMap", "> g_VerticalShadowMap");
        preCode = preCode.Replace("4> g_ShadowMap", "> g_ShadowMap");

        void ConvertToLinear(string texName)
        {
            string search = $"{texName}.Sample";
            int index = -1;

            while ((index = code.IndexOf(search, index + 1, StringComparison.Ordinal)) != -1)
            {
                code = code[..index] + "SrgbToLinear(" + code[index..];

                // Insert an extra paranthesis
                index = code.IndexOf(");", index);
                code = code[..index] + ")" + code[index..];
            }
        }

        void ScaleByLuminance(string paramName) =>
            code = code.Replace(paramName, $"({paramName} * GetToneMapLuminanceVec4())");

        // Convert textures to linear space.
        ConvertToLinear("sampDif");
        ConvertToLinear("sampSpc");
        ConvertToLinear("sampSpec");
        ConvertToLinear("sampFalloff");
        ConvertToLinear("sampEmi");
        ConvertToLinear("sampEnv");
        ConvertToLinear("s0");

        // Divide light color by PI.
        code = code.Replace("mrgGlobalLight_Diffuse", "(mrgGlobalLight_Diffuse / PI)");

        // Nullify omni lights.
        for (int i = 0; i < 4; i++)
            code = code.Replace($"mrgLocalLight{i}_Color", "float4(0, 0, 0, 0)");

        // Scale light field by luminance.
        for (int i = 0; i < 6; i++)
            ScaleByLuminance($"g_aLightField[{i}]");

        // Scale eye highlight color by luminance.
        ScaleByLuminance("g_SonicEyeHighLightColor");

        // Scale mrgLuminanceRange by luminance.
        ScaleByLuminance("mrgLuminanceRange");

        // Scale particles by average luminance.
        if (name.Contains("Effect", StringComparison.Ordinal) ||
            name.Contains("Particle", StringComparison.Ordinal))
            ScaleByLuminance("g_ForceAlphaColor");

        // Scale mrgMaterialColor by luminance.
        if (!name.Contains("SysShading"))
            ScaleByLuminance("mrgMaterialColor");

        // Scale SysError by luminance.
        if (name.Contains("SysError"))
            ScaleByLuminance("C[0].xyyx");

        // Patch shadows.
        codeOriginal = codeOriginal.Replace("g_VerticalShadowMapSampler.SampleCmp(", "GetShadow(g_VerticalShadowMapSampler,");
        codeOriginal = codeOriginal.Replace("g_ShadowMapSampler.SampleCmp(", "GetShadow(g_ShadowMapSampler,");

        code = code.Replace("g_VerticalShadowMap", "1.0; //"); // No vertical shadows on the PBR side
        code = code.Replace("g_ShadowMapSampler.SampleCmp(", "GetShadow(g_ShadowMapSampler,");

        // Remove the 0-4 color clamp.
        code = code.Replace("(4,", "(asfloat(0x7F800000),");
        code = code.Replace(", 4,", ", asfloat(0x7F800000),");
        code = code.Replace(", 4)", ", asfloat(0x7F800000))");

        // Pass correct data to GBuffer.
        index = code.IndexOf("if (enable_alpha_test)", StringComparison.Ordinal);
        index = code.LastIndexOf(';', index) + 1;

        code = code[..index] +
               "float alpha = oC0.w;" +
               "StoreParams(oC0.rgb, 0, 0, oC0, oC1, oC2, oC3);" +
               "oC0.w = alpha;" +
               code[index..];

        // Make the modified code only execute when the PBR toggle is enabled.
        var codeSplit = code.Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        var codeOrgSplit = codeOriginal.Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

        if (codeSplit.Length != codeOrgSplit.Length)
            throw new Exception("Line counts in original and modified codes are somehow different!");

        var stringBuilder = new StringBuilder();
        stringBuilder.Append(preCode);
        stringBuilder.Append(mainCodeArgs);

        for (int begin = 0; begin < codeSplit.Length;)
        {
            if (codeSplit[begin] == codeOrgSplit[begin])
            {
                stringBuilder.AppendFormat("{0}\n", codeSplit[begin++]);
                continue;
            }

            int end = begin + 1;

            while (end < codeSplit.Length && codeSplit[end] != codeOrgSplit[end])
                end++;

            stringBuilder.AppendLine("if (g_UsePBR) {");
            for (int i = begin; i < end; i++)
                stringBuilder.AppendFormat("{0}\n", codeSplit[i]);

            stringBuilder.AppendLine("} else {");
            for (int i = begin; i < end; i++)
                stringBuilder.AppendFormat("{0}\n", codeOrgSplit[i]);

            stringBuilder.AppendLine("}");

            begin = end;
        }

        translated = stringBuilder.ToString();

        bytes = Encoding.UTF8.GetBytes(translated);

        fixed (byte* ptr = bytes)
        {
            using var include = mIncludeFactory.Create(mShaderSourceCodeDirectory);

            int result = D3DCompiler.Compile(
                new IntPtr(ptr),
                bytes.Length,
                null,
                null,
                include.Pointer,
                "main",
                "ps_5_0",
                0,
                0,
                out var blob,
                out var errorBlob);

            if (result < 0)
                throw new Exception(errorBlob.ConvertToString());

            return blob.ToArray();
        }
    }

    public void Dispose()
    {
        mIncludeFactory.Dispose();
    }
}