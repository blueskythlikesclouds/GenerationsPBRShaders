using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoRing : IPixelShaderInfo
    {
        public virtual string Name { get; } = "Ring";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new PixelShaderTechniqueInfo[]
        {
            PixelShaderTechniqueInfoDefault2.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "diffuseSampler", "diffuse", "d" ),
            new SamplerInfo( "emissionSampler", "emission", "d" ),
        };

        public virtual IReadOnlyList<string> Constants { get; } = new[]
        {
            "PBRFactor",
            "Blend"
        };

        public virtual IReadOnlyList<string> Definitions { get; } = new[] { "NoGIOnly" };

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have both samplers
            return samplerBits == 0b11;
        }
    }
}