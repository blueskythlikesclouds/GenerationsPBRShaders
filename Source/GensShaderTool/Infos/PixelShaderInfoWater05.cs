using System;
using System.Collections.Generic;
using Amicitia.IO;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoWater05 : IPixelShaderInfo
    {
        public virtual string Name { get; } = "Water05";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new[]
        {
            PixelShaderTechniqueInfoWater2.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "diffuseSampler", "diffuse", "" ),
            new SamplerInfo( "specularSampler", "specular", "" ), 
            new SamplerInfo( "normalSampler", "normal", "" ), 
            new SamplerInfo( "normal1Sampler", "normal", "" ),
        };

        public virtual IReadOnlyList<string> Constants { get; } = new[]
        {
            "RefractionCubemap"
        };

        public virtual IReadOnlyList<string> Definitions { get; } = new[] { "GIOnly" };

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have all samplers
            return ( samplerBits & 0b1111 ) == 0b111;
        }
    }
}