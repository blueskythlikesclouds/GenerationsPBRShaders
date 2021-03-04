namespace GensShaderTool.Infos
{
    public class PixelShaderTechniqueInfoWater2 : PixelShaderTechniqueInfo
    {
        private static readonly VertexShaderPermutation[] sVertexShaderPermutations =
        {
            new VertexShaderPermutation( VertexShaderSubPermutations.All, "none", "Water2" )
        };

        public static PixelShaderTechniqueInfoWater2 Instance { get; } =
            new PixelShaderTechniqueInfoWater2();

        public PixelShaderTechniqueInfoWater2() : base( "default", "", sVertexShaderPermutations )
        {
        }
    }
}