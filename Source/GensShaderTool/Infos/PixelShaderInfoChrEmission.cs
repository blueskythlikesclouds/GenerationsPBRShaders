using System;
using System.Collections.Generic;
using Amicitia.IO;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoChrEmission : IPixelShaderInfo
    {
        public virtual string Name { get; } = "ChrEmission";

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
            new SamplerInfo( "falloffSampler", "falloff", "f" ),
            new SamplerInfo( "emissionSampler", "emission", "E" ),
        };

        public virtual IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public virtual int IterationCount { get; } = 1;

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have diffuse and emission
            if ( ( samplerBits & 0b10001 ) != 0b10001 )
                return false;

            // Have correct techniques
            bool hasNormal = ( samplerBits & 0b100 ) != 0;
            if ( hasNormal && !( technique is PixelShaderTechniqueInfoDefault2Normal ) )
                return false;

            return hasNormal || technique is PixelShaderTechniqueInfoDefault2;
        }
    }

    public class PixelShaderInfoMChrEmission : PixelShaderInfoChrEmission
    {
        public override string Name { get; } = "MChrEmission";

        public override IReadOnlyList<string> Definitions { get; } = new[] { "HasMetalness" };

        public override bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have specular
            return ( samplerBits & 0b10 ) == 0b10 && base.ValidatePermutation( samplerBits, technique );
        }
    }
}