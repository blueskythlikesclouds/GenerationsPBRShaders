using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoChrEyeCDRF : IPixelShaderInfo
    {
        public virtual string Name { get; } = "ChrEyeCDRF";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new[]
        {
            PixelShaderTechniqueInfoEye2.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "diffuseSampler", "diffuse", "" ),
            new SamplerInfo( "specularSampler", "specular", "" ),
            new SamplerInfo( "cdrSampler", "cdr", "" ),
            new SamplerInfo( "reflectionSampler", "reflection", "" ),
        };

        public virtual IReadOnlyList<string> Definitions { get; } = new[] { "NoGIOnly", "HasMetalness" }; // HasMetalness to prevent rejection of specular W

        public virtual int IterationCount { get; } = 1;

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Have only one variation with all samplers
            return samplerBits == 0b1111;
        }
    }
}