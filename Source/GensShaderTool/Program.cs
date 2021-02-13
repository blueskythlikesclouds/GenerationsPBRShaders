using System;
using System.Globalization;
using System.IO;
using System.Linq;
using GensShaderTool.Infos;

namespace GensShaderTool
{
    internal class Program
    {
        private static readonly string sProjectDirectory =
            "D:\\Repositories\\GenerationsPBRShaders\\Source\\GensShaderTool";

        private static readonly string sOutputDirectory =
            "D:\\Steam\\steamapps\\common\\Sonic Generations\\mods\\PBR Shaders\\disk\\bb3\\shader_r";

        private static void Main(string[] args)
        {
            var (vertexShaderGlobalParameterSet, pixelShaderGlobalParameterSet) = ProcessGlobalShaderParameterSets();

            //========================//
            // Default Vertex Shader //
            //========================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Default.wvu.hlsl"),
                sOutputDirectory, new IShaderInfo[]
                {
                    new VertexShaderInfoDefault2(), new VertexShaderInfoDefault2Normal(), new VertexShaderInfoEye2(), new VertexShaderInfoWater2()
                }, vertexShaderGlobalParameterSet);

            //======================//
            // Default Pixel Shader //
            //======================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Default.wpu.hlsl"),
                sOutputDirectory, new IShaderInfo[]
                {
                    new PixelShaderInfoCommon2(), new PixelShaderInfoBlend2(), new PixelShaderInfoChrEyeCDRF(), new PixelShaderInfoChrSkinCDRF(),
                    new PixelShaderInfoMCommon(), new PixelShaderInfoMBlend(), new PixelShaderInfoWater01(), new PixelShaderInfoWater05(), new PixelShaderInfoRing(), 
                    new PixelShaderInfoEmission(), new PixelShaderInfoGlass2(), new PixelShaderInfoMEmission(), new PixelShaderInfoChrGlass(), new PixelShaderInfoDry()
                }, pixelShaderGlobalParameterSet);

            //===========================//
            // Ignore Light Pixel Shader //
            //===========================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "IgnoreLight.wpu.hlsl"),
                sOutputDirectory,
                new[] { new PixelShaderInfoIgnoreLight2() }, pixelShaderGlobalParameterSet);

            //==================//
            // Sky Pixel Shader //
            //==================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Sky.wpu.hlsl"),
                sOutputDirectory,
                new[] { new PixelShaderInfoSky2() }, pixelShaderGlobalParameterSet);

            //============//
            // LUT Shader //
            //============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "LUT.wpu.hlsl"),
                sOutputDirectory,
                new[] { new PixelShaderInfoLUT() }, pixelShaderGlobalParameterSet);

            //===========================//
            // Convolution Filter Shader //
            //===========================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "ConvolutionFilter.wpu.hlsl"),
                sOutputDirectory,
                new[] { new PixelShaderInfoConvolutionFilter() }, pixelShaderGlobalParameterSet);        
            
            //============================//
            // Deferred Light Pass Shader //
            //============================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Deferred", "LightPass.wpu.hlsl"),
                sOutputDirectory,
                new[] { new PixelShaderInfoDeferredPassLight() }, pixelShaderGlobalParameterSet);            

            //============//
            // RLR Shader //
            //============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "RLR.wpu.hlsl"),
                sOutputDirectory,
                new[] { new PixelShaderInfoRLR() }, pixelShaderGlobalParameterSet);          

            //===============================//
            // Deferred Specular Pass Shader //
            //===============================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Deferred", "IBLPass.wpu.hlsl"),
                sOutputDirectory,
                new[] { new PixelShaderInfoDeferredPassIBL() }, pixelShaderGlobalParameterSet);
            
            //=============//
            // SSAO Shader //
            //=============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "SSAO.wpu.hlsl"),
                sOutputDirectory,
                new[] { new PixelShaderInfoSSAO() }, pixelShaderGlobalParameterSet);
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
    }
}