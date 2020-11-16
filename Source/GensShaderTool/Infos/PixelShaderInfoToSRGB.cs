using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoToSRGB : IPixelShaderInfo
    {
        public string Name { get; } = "FxToSRGB";

        public IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = Array.Empty<PixelShaderTechniqueInfo>();

        public IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "s0", "diffuse", "" ),
        };

        public IReadOnlyList<string> Constants { get; } = Array.Empty<string>();

        public IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            return samplerBits == 0;
        }
    }
}