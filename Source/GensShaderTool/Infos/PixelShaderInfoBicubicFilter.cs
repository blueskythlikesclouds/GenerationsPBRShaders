using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoBicubicFilter : IPixelShaderInfo
    {
        public string Name { get; } = "FxBicubicFilter";

        public IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = Array.Empty<PixelShaderTechniqueInfo>();

        public IReadOnlyList<SamplerInfo> Samplers { get; } = Array.Empty<SamplerInfo>();

        public IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public virtual int IterationCount { get; } = 1;

        public bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            return samplerBits == 0;
        }
    }
}