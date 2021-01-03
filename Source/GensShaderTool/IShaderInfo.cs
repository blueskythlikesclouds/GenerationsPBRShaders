using System;
using System.Collections.Generic;

namespace GensShaderTool
{
    public interface IShaderInfo
    {
        public string Name { get; }
        public ShaderType Type { get; }
        public IReadOnlyList<PixelShaderTechniqueInfo> Techniques { get; }
        public IReadOnlyList<SamplerInfo> Samplers { get; }
        public IReadOnlyList<string> Constants { get; }
        public IReadOnlyList<string> Definitions { get; }
        public int IterationCount { get; }

        public bool ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique );
    }

    public interface IPixelShaderInfo : IShaderInfo
    {
        ShaderType IShaderInfo.Type => ShaderType.Pixel;
    }

    public interface IVertexShaderInfo : IShaderInfo
    {
        ShaderType IShaderInfo.Type => ShaderType.Vertex;

        IReadOnlyList<PixelShaderTechniqueInfo> IShaderInfo.Techniques => Array.Empty<PixelShaderTechniqueInfo>();
        IReadOnlyList<SamplerInfo> IShaderInfo.Samplers => Array.Empty<SamplerInfo>();

        bool IShaderInfo.ValidatePermutation( ushort samplerBits, PixelShaderTechniqueInfo technique )
        {
            return true;
        }
    }
}