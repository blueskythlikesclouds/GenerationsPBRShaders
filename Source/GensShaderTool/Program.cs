using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using GensShaderTool.Infos;
using Vortice.D3DCompiler;

namespace GensShaderTool
{
    internal class Program
    {
        private const int cShaderArPadding = 16;
        private const int cShaderArMaxSplitSize = 5 * 1024 * 1024;

        private static readonly string sProjectDirectory =
            "D:\\Repositories\\GenerationsPBRShaders\\Source\\GensShaderTool";

        private static readonly string sOutputDirectory =
            "D:\\Steam\\steamapps\\common\\Sonic Generations\\mods\\PBR Shaders\\disk\\bb3";

        private static readonly string sBB3Directory =
            "D:\\Steam\\steamapps\\common\\Sonic Generations\\disk\\bb3";

        #if DEBUG
        private const ShaderFlags cShaderFlags = ShaderFlags.SkipOptimization;
        #else
        private const ShaderFlags cShaderFlags = ShaderFlags.OptimizationLevel1;
        #endif

        private static void Main(string[] args)
        {
            if (args.Length > 0)
            {
                if (args[0].EndsWith(".wpu", StringComparison.OrdinalIgnoreCase) ||
                    args[0].EndsWith(".wvu", StringComparison.OrdinalIgnoreCase))
                {
                    ShaderTranslator.Translate(args[0], args[0] + ".hlsl");
                    return;
                }

                if (args[0].EndsWith(".hlsl", StringComparison.OrdinalIgnoreCase))
                {
                    ShaderCompiler.CompileFromFile(args[0],
                        args[0].EndsWith(".wvu.hlsl", StringComparison.OrdinalIgnoreCase)
                            ? ShaderType.Vertex
                            : ShaderType.Pixel, args[0].Substring(0, args[0].Length - 5), cShaderFlags);

                    return;
                }
            }

            string vanillaShaderArPath = Path.Combine(sOutputDirectory, "shader_vanilla.ar.00");
            if (!File.Exists(vanillaShaderArPath))
            {
                var gensShaderDatabase = ConvertShadersToPBRCompatible();

                gensShaderDatabase.Sort();
                gensShaderDatabase.Save(vanillaShaderArPath, cShaderArPadding, cShaderArMaxSplitSize);
            }

            var (vertexShaderGlobalParameterSet, pixelShaderGlobalParameterSet) = ProcessGlobalShaderParameterSets();

            string pbrShaderArPath = Path.Combine(sOutputDirectory, "shader_pbr.ar.00");

            var pbrShaderDatabase = new ArchiveDatabase();
            if (File.Exists(pbrShaderArPath))
                pbrShaderDatabase.Load(pbrShaderArPath);

            //========================//
            // Default Vertex Shader //
            //========================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Default.wvu.hlsl"),
                pbrShaderDatabase, new IShaderInfo[]
                {
                    new VertexShaderInfoDefault2(), new VertexShaderInfoDefault2Normal(), new VertexShaderInfoEye2(),
                    new VertexShaderInfoWater2(), new VertexShaderInfoDefault2NoV()
                }, vertexShaderGlobalParameterSet, cShaderFlags);

            //======================//
            // Default Pixel Shader //
            //======================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Default.wpu.hlsl"),
                pbrShaderDatabase, new IShaderInfo[]
                {
                    new PixelShaderInfoCommon2(), new PixelShaderInfoBlend2(), new PixelShaderInfoChrEyeCDRF(),
                    new PixelShaderInfoChrSkinCDRF(),
                    new PixelShaderInfoMCommon(), new PixelShaderInfoMBlend(), new PixelShaderInfoWater01(),
                    new PixelShaderInfoWater05(), new PixelShaderInfoRing2(),
                    new PixelShaderInfoEmission(), new PixelShaderInfoGlass2(), new PixelShaderInfoMEmission(),
                    new PixelShaderInfoChrGlass(), new PixelShaderInfoDry(), new PixelShaderInfoFalloff2(), new PixelShaderInfoMFalloff(),
                    new PixelShaderInfoChrEmission(), new PixelShaderInfoMChrEmission(),
                    new PixelShaderInfoPointMarker(), new PixelShaderInfoSuperSonic(), new PixelShaderInfoChrEyeSuper()
                }, pixelShaderGlobalParameterSet, cShaderFlags);

            //===========================//
            // Ignore Light Pixel Shader //
            //===========================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "IgnoreLight.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoIgnoreLight2() }, pixelShaderGlobalParameterSet, cShaderFlags);

            //==================//
            // Sky Pixel Shader //
            //==================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Sky.wpu.hlsl"),
                pbrShaderDatabase,
                new IShaderInfo[] { new PixelShaderInfoSky2(), new PixelShaderInfoSky3() }, pixelShaderGlobalParameterSet, cShaderFlags);

            //============//
            // LUT Shader //
            //============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "LUT.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoLUT() }, pixelShaderGlobalParameterSet, cShaderFlags);

            //===========================//
            // Convolution Filter Shader //
            //===========================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "ConvolutionFilter.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoConvolutionFilter() }, pixelShaderGlobalParameterSet, cShaderFlags);

            //============================//
            // Deferred Light Pass Shader //
            //============================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Deferred", "LightPass.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoDeferredPassLight() }, pixelShaderGlobalParameterSet, cShaderFlags);

            //============//
            // RLR Shader //
            //============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "RLR.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoRLR() }, pixelShaderGlobalParameterSet, cShaderFlags);

            //===============================//
            // Deferred Specular Pass Shader //
            //===============================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Deferred", "IBLProbePass.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoDeferredPassIBLProbe() }, pixelShaderGlobalParameterSet, cShaderFlags);

            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Deferred", "IBLCombinePass.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoDeferredPassIBLCombine() }, pixelShaderGlobalParameterSet, cShaderFlags);

            //=============//
            // SSAO Shader //
            //=============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "SSAO.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoSSAO() }, pixelShaderGlobalParameterSet, cShaderFlags);

            //====================//
            // SSAO Filter Shader //
            //====================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "SSAOFilter.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoSSAOFilter() }, pixelShaderGlobalParameterSet, cShaderFlags);

            //==============//
            // Bloom Shader //
            //==============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "Bloom.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoBloom() }, pixelShaderGlobalParameterSet, cShaderFlags);

            pbrShaderDatabase.Sort();
            pbrShaderDatabase.Save(pbrShaderArPath, cShaderArPadding, cShaderArMaxSplitSize);
        }

        private static (ShaderParameterSet vertexShaderParameterSet, ShaderParameterSet pixelShaderParameterSet)
            ProcessGlobalShaderParameterSets()
        {
            string directoryName = "Generations";

            var pixelShaderGlobalParameterSet =
                BinarySerializableEx.Load<ShaderParameterSet>( Path.Combine( sProjectDirectory, "Parameters",
                    directoryName, "global.psparam" ) );

            ProcessGlobalShaderParameterSet( pixelShaderGlobalParameterSet );

            string outputPsParamHlslDirectory = Path.Combine( sProjectDirectory, "global.psparam.hlsl" );

            if ( !File.Exists( outputPsParamHlslDirectory ) )
            {
                using var writer = File.CreateText( outputPsParamHlslDirectory );
                writer.WriteLine("#ifndef GLOBAL_PSPARAM_HLSL_INCLUDED");
                writer.WriteLine("#define GLOBAL_PSPARAM_HLSL_INCLUDED");

                ShaderParameterConverter.WriteHlslRegisters( pixelShaderGlobalParameterSet, writer );

                writer.WriteLine("#endif");
            }

            var vertexShaderGlobalParameterSet =
                BinarySerializableEx.Load<ShaderParameterSet>( Path.Combine( sProjectDirectory, "Parameters",
                    directoryName, "global.vsparam" ) );

            ProcessGlobalShaderParameterSet( vertexShaderGlobalParameterSet );

            string outputVsParamHlslDirectory = Path.Combine( sProjectDirectory, "global.vsparam.hlsl" );

            if ( !File.Exists( outputVsParamHlslDirectory ) )
            {
                using var writer = File.CreateText( outputVsParamHlslDirectory );
                writer.WriteLine("#ifndef GLOBAL_VSPARAM_HLSL_INCLUDED");
                writer.WriteLine("#define GLOBAL_VSPARAM_HLSL_INCLUDED");

                ShaderParameterConverter.WriteHlslRegisters( vertexShaderGlobalParameterSet, writer );

                writer.WriteLine("#endif");
            }

            return ( vertexShaderGlobalParameterSet, pixelShaderGlobalParameterSet );
        }

        private static void ProcessGlobalShaderParameterSet( ShaderParameterSet parameterSet )
        {
            foreach ( var parameter in parameterSet.SingleParameters.Where( parameter =>
                !parameter.Name.StartsWith( "g_", StringComparison.OrdinalIgnoreCase ) &&
                !parameter.Name.StartsWith( "mrg", StringComparison.OrdinalIgnoreCase ) ) )
                parameter.Name = "g_" + CultureInfo.InvariantCulture.TextInfo
                    .ToTitleCase( parameter.Name.Replace( '_', ' ' ) ).Replace( " ", "" );

            foreach ( var parameter in parameterSet.SamplerParameters )
                parameter.Name = $"g_{parameter.Name}Sampler";
        }

        private static ArchiveDatabase ConvertShadersToPBRCompatible()
        {
            var shaderRegular = new ArchiveDatabase(Path.Combine(sBB3Directory, "shader_r.ar.00"));
            var shaderRegularAdd = new ArchiveDatabase(Path.Combine(sBB3Directory, "shader_r_add.ar.00"));

            const string misplacedShaderName = "Common_dn";

            shaderRegularAdd.Contents.AddRange(shaderRegular.Contents.Where(x => x.Name.StartsWith(misplacedShaderName)));
            shaderRegular.Contents.RemoveAll(x => x.Name.StartsWith(misplacedShaderName));

            Parallel.ForEach(
                shaderRegular.Contents.Where(x =>
                    x.Name.Contains("MeasureLuminance", StringComparison.OrdinalIgnoreCase) &&
                    x.Name.EndsWith(".wpu", StringComparison.OrdinalIgnoreCase)),
                x =>
                {
                    x.Data = FixMeasureLuminanceShader(x.Data, x.Name);
                });

            Parallel.ForEach(
                shaderRegularAdd.Contents.Where(x => x.Name.EndsWith(".wpu", StringComparison.OrdinalIgnoreCase)),
                x =>
                {
                    x.Data = ConvertShaderToPBRCompatible(x.Data, x.Name);
                });

            shaderRegular.Contents.AddRange(shaderRegularAdd.Contents);
            return shaderRegular;
        }

        private static byte[] FixMeasureLuminanceShader(byte[] bytes, string debugName)
        {
            string translated = ShaderTranslator.Translate(bytes);

            // Why does ST clamp the colors??? First regular shaders now the tonemapping shader...
            translated = translated.Replace("saturate", "");

            try
            {
                return ShaderCompiler.Compile(translated, ShaderType.Pixel, cShaderFlags);
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed to compile shader {0}: {1}", debugName, e.Message);
                return bytes;
            }
        }

        private static readonly string sFilterHlsl =
            File.ReadAllText(Path.Combine(sProjectDirectory, "Shaders", "Filter.hlsl")).Replace("sampler tex", "sampler2D tex");

        private static byte[] ConvertShaderToPBRCompatible(byte[] bytes, string debugName)
        {
            string translated = ShaderTranslator.Translate(bytes);

            int index = translated.IndexOf("void main(", StringComparison.Ordinal);

            string preCode = translated.Substring(0, index);
            string mainCode = translated.Substring(index);
            mainCode = mainCode.Replace(", 4)", ", 65504)");

            preCode += sFilterHlsl;
            preCode +=
                "\nfloat4 g_ESMParam : register(c110);" +
                "float4 texShadow(sampler2D tex, float4 texCoord)\n" +
                "{\n" +
                "    if (abs(texCoord.x) > texCoord.w || abs(texCoord.y) > texCoord.w || abs(texCoord.z) > texCoord.w)" +
                "        return 1.0;" +
                "    texCoord.xyz /= texCoord.w;\n" +
                "    return texESM(tex, g_ESMParam.xy, texCoord.xy, texCoord.z, g_ESMParam.z).xxxx;\n" +
                "}\n";

            int codeIndex = mainCode.IndexOf('{');
            string mainCodeArgs = mainCode.Substring(0, codeIndex + 1);
            string code = mainCode.Substring(codeIndex + 1);
            string codeOriginal = code;

            // Replace shadowmapping for original code
            codeOriginal = codeOriginal.Replace("texProj(g_ShadowMap", "texShadow(g_ShadowMap");
            codeOriginal = codeOriginal.Replace("texProj(g_VerticalShadowMap", "texShadow(g_VerticalShadowMap");

            // Remove the 0-4 color clamp.
            // TODO: Make this smarter lol.
            code = code.Replace("(4,", "(65504,");
            code = code.Replace(", 4,", ", 65504,");
            code = code.Replace(", 4)", ", 65504)");

            // Divide light color by PI.
            code = code.Replace("mrgGlobalLight_Diffuse",
                "(mrgGlobalLight_Diffuse / " + MathF.PI.ToString(CultureInfo.InvariantCulture) + ")");

            // Nullify omni lights for the time being.
            for (int i = 0; i < 4; i++) 
                code = code.Replace($"mrgLocalLight{i}_Color", "float4(0, 0, 0, 0)");

            // Scale light field by luminance.
            for (int i = 0; i < 6; i++)
                code = code.Replace($"g_aLightField[{i}]",
                    $"(g_aLightField[{i}] * float4(GetToneMapLuminance().xxx, 1))");

            // Scale eye highlight color by luminance.
            code = code.Replace("g_SonicEyeHighLightColor",
                "(g_SonicEyeHighLightColor * float4(GetToneMapLuminance().xxx, 1))");           
            
            // Scale mrgLuminanceRange by luminance.
            code = code.Replace("mrgLuminanceRange",
                "(mrgLuminanceRange * float4(GetToneMapLuminance().xxx, 1))");

            // Convert diffuse to linear space.
            preCode += "float4 g_HDRParam_SGGIParam : register(c109);" +
                       "sampler2D g_LuminanceSampler : register(s8);" +
                       "float GetToneMapLuminance()" +
                       "{" +
                       "    return tex2Dlod(g_LuminanceSampler, 0).x * g_HDRParam_SGGIParam.x;" +
                       "}" +
                       "float4 texSRGB(sampler2D s, float4 texCoord)" +
                       "{ " +
                       "    return pow(tex(s, texCoord), float4(2.2, 2.2, 2.2, 1.0)); " +
                       "}";

            code = code.Replace("tex(sampDif", "texSRGB(sampDif");
            code = code.Replace("tex(s0,", "texSRGB(s0,");

            // Scale ForceAlphaColor/SmokeColor by luminance.
            if (debugName.Contains("Spark", StringComparison.Ordinal) || debugName.Contains("Particle", StringComparison.Ordinal))
                code = code.Replace("g_ForceAlphaColor", "(g_ForceAlphaColor * float4(GetToneMapLuminance().xxx, 1))");

            if (debugName.Contains("SmokeTest", StringComparison.Ordinal))
            {
                // Scale smoke color by luminance.
                code = code.Replace("g_SmokeColor", "(g_SmokeColor * float4(GetToneMapLuminance().xxx, 1))");
                
                // Prevent the smoke color saturation.
                code = code.Replace("oC0.xyz = saturate", "oC0.xyz =");
            }

            // Ignore shadowmaps for the time being.
            code = code.Replace("texProj(g_VerticalShadowMap", "1; //");
            code = code.Replace("texProj(g_ShadowMap", "1; //");

            // Pass correct data to GBuffer.
            code = code[..^4] + "oC1 = 0; oC2 = float4(0, 1, 0, 1); oC3 = 0;\n }";

            // Add the g_UsePBR bool
            preCode += "bool g_UsePBR : register(b6);";

            // Make the modified code only execute when the PBR toggle is enabled.
            var codeSplit = code.Split('\n',
                StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

            var codeOrgSplit = codeOriginal.Split('\n',
                StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

            if (codeSplit.Length != codeOrgSplit.Length)
            {
                Console.WriteLine("Line counts in original and modified codes are somehow different! {0}", debugName);
                return bytes;
            }

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

            try
            {
                Console.WriteLine("Compiling {0}", debugName);
                return ShaderCompiler.Compile(stringBuilder.ToString(), ShaderType.Pixel, cShaderFlags);
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed to compile shader {0}: {1}", debugName, e.Message);
                Console.WriteLine(stringBuilder);
                return bytes;
            }
        }
    }
}