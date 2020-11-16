using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoChrEyeCDRF : IPixelShaderInfo
    {
        public virtual string Name { get; } = "ChrEyeCDRF";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new[]
        {
            PixelShaderTechniqueInfoEye2.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "diffuseSampler", "diffuse", "" ),
            new SamplerInfo( "specularSampler", "specular", "" ),
            new SamplerInfo( "cdrSampler", "cdr", "" ),
            new SamplerInfo( "reflectionSampler", "reflection", "" ),
        };

        public virtual IReadOnlyList<string> Constants { get; } = new[]
        {
            "ChrEye1",
            "ChrEye2",
            "ChrEye3"
        };

        public virtual IReadOnlyList<string> Definitions { get; } = new[] { "NoGIOnly" };

        public virtual bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            // Have only one variation with all samplers
            return samplerBits == 0b1111;
        }
    }
}