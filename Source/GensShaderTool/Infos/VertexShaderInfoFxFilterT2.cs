using System;
using System.Collections.Generic;

namespace GensShaderTool.Infos
{
    public class VertexShaderInfoFxFilterPT2 : IVertexShaderInfo
    {
        public string Name { get; } = "FxFilterPT2";

        public IReadOnlyList<string> Constants => Array.Empty<string>();
        public IReadOnlyList<string> Definitions => Array.Empty<string>();
    }
}