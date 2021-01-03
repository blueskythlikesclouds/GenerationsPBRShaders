using System;
using System.Collections.Generic;
using Amicitia.IO;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoInfinite : IPixelShaderInfo
    {
        public virtual string Name { get; } = "Infinite";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new PixelShaderTechniqueInfo[]
        {
            PixelShaderTechniqueInfoDefault2Normal.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "diffuseSampler", "diffuse", "" ),
            new SamplerInfo( "specularSampler", "specular", "" ),
            new SamplerInfo( "normalSampler", "normal", "" ),
            new SamplerInfo( "falloffSampler", "falloff", "" ),
            new SamplerInfo( "reflectionSampler", "reflection", "" ),
        };

        public virtual IReadOnlyList<string> Constants { get; } = new[]
        {
            "FalloffFactor",
            "FalloffFactorE"
        };

        public virtual IReadOnlyList<string> Definitions { get; } = new[] { "NoGIOnly" };

        public virtual int IterationCount { get; } = 1;

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Have only one variation with all samplers
            return samplerBits == 0b11111;
        }
    }
}