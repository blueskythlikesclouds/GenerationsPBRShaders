using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoVolumetricLightingIgnoreSky : IPixelShaderInfo
    {
        public string Name { get; } = "FxVolumetricLighting_IgnoreSky";

        public IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = Array.Empty<PixelShaderTechniqueInfo>();

        public IReadOnlyList<SamplerInfo> Samplers { get; } = Array.Empty<SamplerInfo>();

        public IReadOnlyList<string> Definitions { get; } = new[] { "IgnoreSky" };

        public virtual int IterationCount { get; } = 1;

        public bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            return samplerBits == 0;
        }
    }
}