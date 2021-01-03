using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoDeferredPassLight : IPixelShaderInfo
    {
        public string Name { get; } = "FxDeferredPassLight";

        public IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = Array.Empty<PixelShaderTechniqueInfo>();

        public IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "g_GBuffer0Sampler", "GBuffer0", "" ),
            new SamplerInfo( "g_GBuffer1Sampler", "GBuffer1", "" ),
            new SamplerInfo( "g_GBuffer2Sampler", "GBuffer2", "" ),
            new SamplerInfo( "g_GBuffer3Sampler", "GBuffer3", "" ),
        };

        public IReadOnlyList<string> Constants { get; } = Array.Empty<string>();

        public IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public virtual int IterationCount { get; } = 1 + 32;

        public bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            return samplerBits == 0;
        }
    }
}