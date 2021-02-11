using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class PixelShaderInfoIgnoreLight2 : IPixelShaderInfo
    {
        public virtual string Name { get; } = "IgnoreLight2";

        public virtual IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; } = new PixelShaderTechniqueInfo[]
        {
            PixelShaderTechniqueInfoDefault2.Instance
        };

        public virtual IReadOnlyList<SamplerInfo> Samplers { get; } = new[]
        {
            new SamplerInfo( "diffuseSampler", "diffuse", "d" ),
            new SamplerInfo( "emissionSampler", "emission", "E" ),
            new SamplerInfo( "emissionReflectionSampler", "emission", "E1" ),
            new SamplerInfo( "transparencySampler", "transparency", "a" ),
            new SamplerInfo( "reflectionSampler", "reflection", "o" )
        };

        public virtual IReadOnlyList<string> Definitions { get; } = Array.Empty<string>();

        public virtual int IterationCount { get; } = 1;

        public virtual bool ValidatePermutation(ushort samplerBits, PixelShaderTechniqueInfo technique)
        {
            bool valid = false;

            valid |= samplerBits == 0b00001; // d
            valid |= samplerBits == 0b01001; // da
            valid |= samplerBits == 0b11001; // dao
            valid |= samplerBits == 0b00011; // dE
            valid |= samplerBits == 0b00010; // E
            valid |= samplerBits == 0b00100; // E1
            valid |= samplerBits == 0b11010; // Eao

            return valid;
        }
    }
}