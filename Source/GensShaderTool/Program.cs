using System;
using System.Globalization;
using System.IO;
using System.Linq;
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
        private const ShaderFlags cShaderFlags = ShaderFlags.OptimizationLevel3;
        #endif

        private static void Main(string[] args)
        {
            string vanillaShaderArPath = Path.Combine(sOutputDirectory, "shader_vanilla.ar.00");
            if (!File.Exists(vanillaShaderArPath))
            {
                var gensShaderDatabase = ConvertShadersToPBRCompatible();

                gensShaderDatabase.Sort();
                gensShaderDatabase.Save(vanillaShaderArPath, cShaderArPadding, cShaderArMaxSplitSize);
            }

            var (vertexShaderGlobalParameterSet, pixelShaderGlobalParameterSet) = ProcessGlobalShaderParameterSets();

            var pbrShaderDatabase = new ArchiveDatabase();

            //========================//
            // Default Vertex Shader //
            //========================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Default.wvu.hlsl"),
                pbrShaderDatabase, new IShaderInfo[]
                {
                    new VertexShaderInfoDefault2(), new VertexShaderInfoDefault2Normal(), new VertexShaderInfoEye2(), new VertexShaderInfoWater2()
                }, vertexShaderGlobalParameterSet, cShaderFlags);

            //======================//
            // Default Pixel Shader //
            //======================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Default.wpu.hlsl"),
                pbrShaderDatabase, new IShaderInfo[]
                {
                    new PixelShaderInfoCommon2(), new PixelShaderInfoBlend2(), new PixelShaderInfoChrEyeCDRF(), new PixelShaderInfoChrSkinCDRF(),
                    new PixelShaderInfoMCommon(), new PixelShaderInfoMBlend(), new PixelShaderInfoWater01(), new PixelShaderInfoWater05(), new PixelShaderInfoRing2(), 
                    new PixelShaderInfoEmission(), new PixelShaderInfoGlass2(), new PixelShaderInfoMEmission(), new PixelShaderInfoChrGlass(), new PixelShaderInfoDry()
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
                new[] { new PixelShaderInfoSky2() }, pixelShaderGlobalParameterSet, cShaderFlags);

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
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Deferred", "IBLPass.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoDeferredPassIBL() }, pixelShaderGlobalParameterSet, cShaderFlags);
            
            //=============//
            // SSAO Shader //
            //=============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "SSAO.wpu.hlsl"),
                pbrShaderDatabase,
                new[] { new PixelShaderInfoSSAO() }, pixelShaderGlobalParameterSet, cShaderFlags);

            pbrShaderDatabase.Sort();
            pbrShaderDatabase.Save(Path.Combine(sOutputDirectory, "shader_pbr.ar.00"), cShaderArPadding, cShaderArMaxSplitSize);
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
                shaderRegularAdd.Contents.Where(x => x.Name.EndsWith(".wpu", StringComparison.OrdinalIgnoreCase)),
                x =>
                {
                    x.Data = ConvertShaderToPBRCompatible(x.Data, x.Name);
                });

            shaderRegular.Contents.AddRange(shaderRegularAdd.Contents);
            return shaderRegular;
        }

        private static readonly string[] sSRGBTokens =
        {
            "sampDif"
        };

        private static byte[] ConvertShaderToPBRCompatible(byte[] bytes, string debugName)
        {
            string translated = ShaderTranslator.Translate(bytes);
            
            translated = translated.Replace("void main(",
                "float4 texSRGB(sampler2D s, float4 texCoord) { return pow(tex(s, texCoord), float4(2.2, 2.2, 2.2, 1.0)); }" +
                "float4 texSRGB(samplerCUBE s, float4 texCoord) { return pow(tex(s, texCoord), float4(2.2, 2.2, 2.2, 1.0)); }" +
                " void main(");

            foreach (string token in sSRGBTokens) 
                translated = translated.Replace($"tex({token}", $"texSRGB({token}");

            // Don't sample shadow maps (for now)
            translated = translated.Replace("texProj(g_VerticalShadowMap", "1; //");
            translated = translated.Replace("texProj(g_ShadowMap", "1; //");

            // Pass proper data to GBuffer
            translated = translated[..^3] + "oC1 = 0; oC2 = float4(0, 1, 0, 1); oC3 = 0; }";

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
    }
}