using System;
using System.Globalization;
using System.IO;
using System.Linq;
using GensShaderTool.Infos;

namespace GensShaderTool
{
    internal class Program
    {
        private const bool IS_XBOX_360 = false;

        private static readonly string sProjectDirectory =
            "D:\\Repositories\\GenerationsPBRShaders\\Source\\GensShaderTool";

        private static readonly string sOutputDirectory =
            "D:\\Steam\\steamapps\\common\\Sonic Generations\\mods\\PBR Shaders\\disk\\bb3\\shader_r";

        private static readonly string sOutputXbox360Directory =
            "D:\\Steam\\steamapps\\common\\Sonic Generations\\mods\\PBR Shaders\\disk\\bb3\\shader";

        
        private static void Main(string[] args)
        {
            var (vertexShaderGlobalParameterSet, pixelShaderGlobalParameterSet) = ProcessGlobalShaderParameterSets();

            /*
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "TV.wpu.hlsl"),
                "D:\\Steam\\steamapps\\common\\Sonic Generations\\mods\\Playground\\disk\\bb3\\shader_r",
                new IShaderInfo[] { new PixelShaderInfoTV() }, pixelShaderGlobalParameterSet);
            */

            //========================//
            // Default Vertex Shader //
            //========================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Default.wvu.hlsl"),
                IS_XBOX_360 ? sOutputXbox360Directory : sOutputDirectory, new IShaderInfo[]
                {
                    new VertexShaderInfoDefault2(), new VertexShaderInfoDefault2Normal(), new VertexShaderInfoEye2(), new VertexShaderInfoWater2()
                }, vertexShaderGlobalParameterSet);

            //======================//
            // Default Pixel Shader //
            //======================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Default.wpu.hlsl"),
                IS_XBOX_360 ? sOutputXbox360Directory : sOutputDirectory, new IShaderInfo[]
                {
                    new PixelShaderInfoCommon2(), new PixelShaderInfoBlend2(), new PixelShaderInfoChrEyeCDRF(), new PixelShaderInfoChrSkinCDRF(),
                    new PixelShaderInfoMCommon(), new PixelShaderInfoMBlend(), new PixelShaderInfoWater01(), new PixelShaderInfoWater05(), new PixelShaderInfoRing()
                }, pixelShaderGlobalParameterSet);


            //==================//
            // Sky Pixel Shader //
            //==================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Sky.wpu.hlsl"),
                IS_XBOX_360 ? sOutputXbox360Directory : sOutputDirectory,
                new[] { new PixelShaderInfoSky2() }, pixelShaderGlobalParameterSet);

            //============//
            // LUT Shader //
            //============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "LUT.wpu.hlsl"),
                IS_XBOX_360 ? sOutputXbox360Directory : sOutputDirectory,
                new[] { new PixelShaderInfoLUT() }, pixelShaderGlobalParameterSet);        
            
            //===============//
            // ToSRGB Shader //
            //===============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "ToSRGB.wpu.hlsl"),
                IS_XBOX_360 ? sOutputXbox360Directory : sOutputDirectory,
                new[] { new PixelShaderInfoToSRGB() }, pixelShaderGlobalParameterSet);             
            
            //===========================//
            // Convolution Filter Shader //
            //===============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "ConvolutionFilter.wpu.hlsl"),
                IS_XBOX_360 ? sOutputXbox360Directory : sOutputDirectory,
                new[] { new PixelShaderInfoConvolutionFilter() }, pixelShaderGlobalParameterSet);        
            
            //==============================//
            // Deferred Terrain Pass Shader //
            //==============================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Deferred", "TerrainPass.wpu.hlsl"),
                IS_XBOX_360 ? sOutputXbox360Directory : sOutputDirectory,
                new[] { new PixelShaderInfoDeferredPassTerrain() }, pixelShaderGlobalParameterSet);

            //=============================//
            // Deferred Object Pass Shader //
            //=============================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Deferred", "ObjectPass.wpu.hlsl"),
                IS_XBOX_360 ? sOutputXbox360Directory : sOutputDirectory,
                new[] { new PixelShaderInfoDeferredPassObject() }, pixelShaderGlobalParameterSet);            

            //============//
            // RLR Shader //
            //============//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Filter", "RLR.wpu.hlsl"),
                IS_XBOX_360 ? sOutputXbox360Directory : sOutputDirectory,
                new[] { new PixelShaderInfoRLR() }, pixelShaderGlobalParameterSet);          

            //===============================//
            // Deferred Specular Pass Shader //
            //===============================//
            ShaderCompiler.Compile(Path.Combine(sProjectDirectory, "Shaders", "Deferred", "IBLPass.wpu.hlsl"),
                IS_XBOX_360 ? sOutputXbox360Directory : sOutputDirectory,
                new[] { new PixelShaderInfoDeferredPassIBL() }, pixelShaderGlobalParameterSet);
        }
        

        private static (ShaderParameterSet vertexShaderParameterSet, ShaderParameterSet pixelShaderParameterSet)
            ProcessGlobalShaderParameterSets()
        {
            string directoryName = IS_XBOX_360 ? "Unleashed" : "Generations";

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