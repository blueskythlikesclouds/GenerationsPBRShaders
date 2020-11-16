using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoLUT : IPixelShaderInfo
    {
        public string Name { get; } = "FxColorCorrectionLUT";

        public IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = Array.Empty<PixelShaderTechniqueInfo>();

        public IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "g_FramebufferSampler", "Framebuffer", "" ),
            new SamplerInfo( "g_LUTSampler", "LUT", "" )
        };

        public IReadOnlyList<string> Constants { get; } = new[]
        {
            "g_LUTParam"
        };

        public IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            return samplerBits == 0;
        }
    }
}