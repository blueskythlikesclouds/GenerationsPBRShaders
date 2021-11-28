using System;
using System.Collections.Generic;
using Amicitia.IO;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoTriplanarBlend : IPixelShaderInfo
    {
        public virtual string Name { get; } = "TriplanarBlend";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new PixelShaderTechniqueInfo[]
        {
            PixelShaderTechniqueInfoDefault2Normal.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "diffuseSampler", "diffuse", "d" ),
            new SamplerInfo( "specularSampler", "specular", "p" ),
            new SamplerInfo( "normalSampler", "normal", "n" ),
            new SamplerInfo( "diffuseBlendSampler", "diffuse", "d" ),
            new SamplerInfo( "specularBlendSampler", "specular", "p" ),
            new SamplerInfo( "normalBlendSampler", "normal", "n" ),
            new SamplerInfo( "normalDetailSampler", "normal", "n" ),
        };

        public virtual IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public virtual int IterationCount { get; } = 1;

        public virtual bool ValidatePermutation(ushort samplerBits, PixelShaderTechniqueInfo technique)
        {
            // dpndpn, dpndpnn
            return samplerBits == 0b111111 || samplerBits == 0b1111111;
        }
    }
}