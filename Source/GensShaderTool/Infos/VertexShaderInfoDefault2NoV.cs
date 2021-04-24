using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class VertexShaderInfoDefault2NoV : IVertexShaderInfo
    {
        public string Name { get; } = "Default2NoV";

        public IReadOnlyList<string> Constants => Array.Empty<string>();
        public IReadOnlyList<string> Definitions => new[] { "NoVertexColor" };
        public virtual int IterationCount { get; } = 1;
    }
}