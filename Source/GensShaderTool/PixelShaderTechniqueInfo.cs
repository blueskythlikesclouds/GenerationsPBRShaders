using System.Collections.Generic;

namespace GensShaderTool
{
    public class PixelShaderTechniqueInfo
    {
        public string Name { get; }
        public string Suffix { get; }
        public IReadOnlyList<VertexShaderPermutation> VertexShaderPermutations { get; }

        public PixelShaderTechniqueInfo( string name, string suffix,
            IEnumerable<VertexShaderPermutation> vertexShaderPermutations )
        {
            Name = name;
            Suffix = suffix;
            VertexShaderPermutations = new List<VertexShaderPermutation>( vertexShaderPermutations );
        }
    }
}