namespace GensShaderTool.Infos
{
    public class PixelShaderTechniqueInfoDefault2Normal : PixelShaderTechniqueInfo
    {
        private static readonly VertexShaderPermutation[] sVertexShaderPermutations =
        {
            new VertexShaderPermutation( VertexShaderSubPermutations.All, "none", "Default2Normal" )
        };

        public static PixelShaderTechniqueInfoDefault2Normal Instance { get; } =
            new PixelShaderTechniqueInfoDefault2Normal();

        public PixelShaderTechniqueInfoDefault2Normal() : base( "default", "", sVertexShaderPermutations )
        {
        }
    }
}