using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoCommon2 : IPixelShaderInfo
    {
        public virtual string Name { get; } = "Common2";

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
            new SamplerInfo( "transparencySampler", "transparency", "a" ),
        };

        public virtual IReadOnlyList<string> Constants { get; } = new[]
        {
            "PBRFactor"
        };

        public virtual IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have diffuse
            if (( samplerBits & 0b1 ) != 0b1)
                return false;

            // Have correct techniques
            bool hasNormal = ( samplerBits & 0b100 ) != 0;
            if ( hasNormal && !( technique is PixelShaderTechniqueInfoDefault2Normal ) )
                return false;

            return hasNormal || technique is PixelShaderTechniqueInfoDefault2;
        }
    }

    public class PixelShaderInfoMCommon : PixelShaderInfoCommon2
    {
        public override string Name { get; } = "MCommon";

        public override IReadOnlyList<string> Definitions { get; } = new[] { "HasMetalness" };

        public override bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have specular
            return ( samplerBits & 0b10 ) == 0b10 && base.ValidatePermutation( samplerBits, technique );
        }
    }
}