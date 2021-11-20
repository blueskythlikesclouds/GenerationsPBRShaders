using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class VertexShaderInfoDetailBlend : IVertexShaderInfo
    {
        public string Name { get; } = "DetailBlend";

        public IReadOnlyList<string> Constants => Array.Empty<string>();
        public IReadOnlyList<string> Definitions => new[] { "HasNormal", "IsBoneless" };
        public virtual int IterationCount { get; } = 1;
    }
}