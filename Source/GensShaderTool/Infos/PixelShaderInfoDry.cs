using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoDry : IPixelShaderInfo
    {
        public virtual string Name { get; } = "Dry";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new PixelShaderTechniqueInfo[]
        {
            PixelShaderTechniqueInfoDefault2Normal.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "diffuseSampler", "diffuse", "d" ),
            new SamplerInfo( "specularSampler", "specular", "p" ),
            new SamplerInfo( "normalSampler", "normal", "n" ),
            new SamplerInfo( "normalBlendSampler", "normal", "n" ),
        };

        public virtual IReadOnlyList<string> Definitions { get; } = new[] { "UseApproxEnvBRDF" };

        public virtual int IterationCount { get; } = 1;

        public virtual bool ValidatePermutation(ushort samplerBits, PixelShaderTechniqueInfo technique)
        {
            return samplerBits == 0b0101 || // dn
                   samplerBits == 0b0111 || // dpn
                   samplerBits == 0b1111; // dpnn
        }
    }
}