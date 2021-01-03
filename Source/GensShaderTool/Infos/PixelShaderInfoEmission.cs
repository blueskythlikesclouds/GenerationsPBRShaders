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

        public virtual int IterationCount { get; } = 1;

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

    public class PixelShaderInfoMEmission : PixelShaderInfoEmission
    {
        public override string Name { get; } = "MEmission";

        public override IReadOnlyList<string> Definitions { get; } = new[] { "HasMetalness" };

        public override bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have specular
            return ( samplerBits & 0b10 ) == 0b10 && base.ValidatePermutation( samplerBits, technique );
        }
    }
}