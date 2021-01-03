using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class VertexShaderInfoDefault2Normal : IVertexShaderInfo
    {
        public string Name { get; } = "Default2Normal";

        public IReadOnlyList<string> Constants => Array.Empty<string>();
        public IReadOnlyList<string> Definitions => Array.Empty<string>();
        public virtual int IterationCount { get; } = 1;
    }
}