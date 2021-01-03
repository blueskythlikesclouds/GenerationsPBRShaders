using System;
using System.Collections.Generic;
using Amicitia.IO;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoBlend2 : IPixelShaderInfo
    {
        public virtual string Name { get; } = "Blend2";

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
            new SamplerInfo( "blendSampler", "opacity", "b" ),
            new SamplerInfo( "diffuseBlendSampler", "diffuse", "d" ),
            new SamplerInfo( "specularBlendSampler", "specular", "p" ),
            new SamplerInfo( "normalBlendSampler", "normal", "n" ),
        };

        public virtual IReadOnlyList<string> Constants { get; } = new[]
        {
            "PBRFactor"
        };

        public virtual IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public virtual int IterationCount { get; } = 1;

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have diffuse
            if ( ( samplerBits & 1 ) == 0 )
                return false;

            ulong upperBound = BitHelper.Unpack( samplerBits, 4, 6 );

            // Upper bound should always exist
            if ( upperBound == 0 )
                return false;

            // Upper bound must coexist with lower bound
            if ( ( samplerBits & upperBound ) != upperBound )
                return false;

            // Have correct techniques
            bool hasNormal = ( samplerBits & 0b1000100 ) != 0;
            if ( hasNormal && !( technique is PixelShaderTechniqueInfoDefault2Normal ) )
                return false;

            return hasNormal || technique is PixelShaderTechniqueInfoDefault2;
        }
    }

    public class PixelShaderInfoMBlend : PixelShaderInfoBlend2
    {
        public override string Name { get; } = "MBlend";

        public override IReadOnlyList<string> Definitions { get; } = new[] { "HasMetalness" };

        public override bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Always have specular
            return ( samplerBits & 0b10 ) == 0b10 && base.ValidatePermutation( samplerBits, technique );
        }
    }
}