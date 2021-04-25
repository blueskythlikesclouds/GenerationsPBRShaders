using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoRing2 : IPixelShaderInfo
    {
        public virtual string Name { get; } = "Ring2";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new PixelShaderTechniqueInfo[]
        {
            PixelShaderTechniqueInfoDefault2.Instance,
            PixelShaderTechniqueInfoDefault2Normal.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "diffuseSampler", "diffuse", "d" ),
            new SamplerInfo( "emissionSampler", "emission", "d" ),
            new SamplerInfo( "specularSampler", "specular", "p" ),
            new SamplerInfo( "normalSampler", "normal", "n" )
        };

        public virtual IReadOnlyList<string> Definitions { get; } = new[] { "NoGIOnly" };

        public virtual int IterationCount { get; } = 1;

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have diffuse and emission
            if (( samplerBits & 0b11 ) != 0b11)
                return false;

            // Have correct techniques
            bool hasNormal = ( samplerBits & 0b1000 ) != 0;
            if ( hasNormal && !( technique is PixelShaderTechniqueInfoDefault2Normal ) )
                return false;

            return hasNormal || technique is PixelShaderTechniqueInfoDefault2;
        }
    }
}