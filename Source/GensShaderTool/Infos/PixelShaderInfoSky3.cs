using System;
using System.Collections.Generic;
using Amicitia.IO;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoSky3 : IPixelShaderInfo
    {
        public virtual string Name { get; } = "Sky3";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new PixelShaderTechniqueInfo[]
        {
            PixelShaderTechniqueInfoSkyUV.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new SamplerInfo[]
        {
            new SamplerInfo("diffuseSampler", "diffuse", "d"),
            new SamplerInfo("transparencySampler", "transparency", "a"),
            new SamplerInfo("emissionSampler", "emission", "E")
        };

        public virtual IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public virtual int IterationCount { get; } = 1;

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have diffuse
            return (samplerBits & 1) == 1;
        }
    }
}