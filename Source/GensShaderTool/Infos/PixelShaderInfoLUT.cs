using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoLUT : IPixelShaderInfo
    {
        public string Name { get; } = "FxLUT";

        public IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = Array.Empty<PixelShaderTechniqueInfo>();

        public IReadOnlyList<SamplerInfo> Samplers { get; } = Array.Empty<SamplerInfo>();

        public IReadOnlyList<string> Constants { get; } = Array.Empty<string>();

        public IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            return samplerBits == 0;
        }
    }
}