using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoConeTrace : IPixelShaderInfo
    {
        public string Name { get; } = "FxConeTrace";

        public IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = Array.Empty<PixelShaderTechniqueInfo>();

        public IReadOnlyList<SamplerInfo> Samplers { get; } = new SamplerInfo[]
        {
        };

        public IReadOnlyList<string> Constants { get; } = new string[]
        {
        };

        public IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            return samplerBits == 0;
        }
    }
}