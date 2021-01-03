using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class VertexShaderInfoWater2 : IVertexShaderInfo
    {
        public string Name { get; } = "Water2";

        public IReadOnlyList<string> Constants => Array.Empty<string>();
        public IReadOnlyList<string> Definitions { get; } = new[] { "IsBoneless" };
        public virtual int IterationCount { get; } = 1;
    }
}