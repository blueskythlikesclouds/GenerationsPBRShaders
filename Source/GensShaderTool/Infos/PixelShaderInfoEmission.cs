using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoEmission : IPixelShaderInfo
    {
        public virtual string Name { get; } = "Emission";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new PixelShaderTechniqueInfo[]
        {
            PixelShaderTechniqueInfoDefault2.Instance,
            PixelShaderTechniqueInfoDefault2Normal.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "diffuseSampler", "diffuse", "d" ),
            new SamplerInfo( "specularSampler", "specular", "p" ),
            new SamplerInfo( "normalSampler", "normal", "n" ),
            new SamplerInfo( "emissionSampler", "emission", "E" ),
            new SamplerInfo( "transparencySampler", "transparency", "a" ),
        };

        public virtual IReadOnlyList<string> Constants { get; } = new[]
        {
            "PBRFactor",
            "Luminance"
        };

        public virtual IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have diffuse and emission
            if (( samplerBits & 0b1001 ) != 0b1001)
                return false;

            // Have correct techniques
            bool hasNormal = ( samplerBits & 0b100 ) != 0;
            if ( hasNormal && !( technique is PixelShaderTechniqueInfoDefault2Normal ) )
                return false;

            return hasNormal || technique is PixelShaderTechniqueInfoDefault2;
        }
    }
}