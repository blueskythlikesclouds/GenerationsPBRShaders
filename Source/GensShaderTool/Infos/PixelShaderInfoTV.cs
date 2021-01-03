using System;
using System.Collections.Generic;
using Amicitia.IO;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoTV : IPixelShaderInfo
    {
        public virtual string Name { get; } = "TV";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new PixelShaderTechniqueInfo[]
        {
            PixelShaderTechniqueInfoDefault.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new SamplerInfo[]
        {
        };

        public virtual IReadOnlyList<string> Constants { get; } = new string[]
        {
        };

        public virtual IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public virtual int IterationCount { get; } = 1;

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            return samplerBits == 0;
        }
    }
}